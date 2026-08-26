#include "playback.h"

#include <errno.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include <ff.h>
#include <zephyr/drivers/i2s.h>
#include <zephyr/fs/fs.h>
#include <zephyr/kernel.h>
#include <zephyr/sys/printk.h>
#include <zephyr/sys/util.h>

#include "app_cmd.h"

#define BLOCK_SIZE 2048
#define NUM_BLOCKS 20
#define SD_MOUNT_POINT "/SD:"

#ifdef CONFIG_NOCACHE_MEMORY
#define MEM_SLAB_CACHE_ATTR __nocache
#else
#define MEM_SLAB_CACHE_ATTR
#endif

static char MEM_SLAB_CACHE_ATTR __aligned(WB_UP(32))
	_k_mem_slab_buf_tx_0_mem_slab[NUM_BLOCKS * WB_UP(BLOCK_SIZE)];

static STRUCT_SECTION_ITERABLE(k_mem_slab, tx_0_mem_slab) =
	Z_MEM_SLAB_INITIALIZER(tx_0_mem_slab, _k_mem_slab_buf_tx_0_mem_slab,
			       WB_UP(BLOCK_SIZE), NUM_BLOCKS);

struct wav_header {
	char riff[4];
	uint32_t file_size;
	char wave[4];
	char fmt_id[4];
	uint32_t fmt_size;
	uint16_t audio_format;
	uint16_t num_channels;
	uint32_t sample_rate;
	uint32_t byte_rate;
	uint16_t block_align;
	uint16_t bits_per_sample;
	char data_id[4];
	uint32_t data_size;
} __packed;

static const struct device *g_i2s_dev;
static FATFS fat_fs;
static struct fs_mount_t g_mount = {
	.type = FS_FATFS,
	.fs_data = &fat_fs,
	.mnt_point = SD_MOUNT_POINT,
};
static bool g_sd_mounted;
static uint8_t g_volume = 100;

static void print_wav_header(const struct wav_header *hdr)
{
	printk("WAV header:\n");
	printk("  riff: %.4s\n", hdr->riff);
	printk("  file_size: %u\n", hdr->file_size);
	printk("  wave: %.4s\n", hdr->wave);
	printk("  fmt_id: %.4s\n", hdr->fmt_id);
	printk("  fmt_size: %u\n", hdr->fmt_size);
	printk("  audio_format: %u\n", hdr->audio_format);
	printk("  num_channels: %u\n", hdr->num_channels);
	printk("  sample_rate: %u\n", hdr->sample_rate);
	printk("  byte_rate: %u\n", hdr->byte_rate);
	printk("  block_align: %u\n", hdr->block_align);
	printk("  bits_per_sample: %u\n", hdr->bits_per_sample);
	printk("  data_id: %.4s\n", hdr->data_id);
	printk("  data_size: %u\n", hdr->data_size);
}

static int mount_sd(void)
{
	int ret;

	if (g_sd_mounted) {
		return 0;
	}

	ret = fs_mount(&g_mount);
	if (ret < 0) {
		printk("fs_mount failed: %d\n", ret);
		return ret;
	}

	g_sd_mounted = true;
	printk("SD card mounted at %s\n", g_mount.mnt_point);
	return 0;
}

static int read_wav_header(struct fs_file_t *file, struct wav_header *hdr)
{
	ssize_t n = fs_read(file, hdr, sizeof(*hdr));

	if (n < 0) {
		printk("fs_read header failed: %d\n", (int)n);
		return (int)n;
	}

	if (n != sizeof(*hdr)) {
		printk("Short WAV header read: got %d bytes, expected %u\n",
		       (int)n, (unsigned int)sizeof(*hdr));
		return -EIO;
	}

	print_wav_header(hdr);

	if (memcmp(hdr->riff, "RIFF", 4) != 0 ||
	    memcmp(hdr->wave, "WAVE", 4) != 0 ||
	    memcmp(hdr->fmt_id, "fmt ", 4) != 0 ||
	    memcmp(hdr->data_id, "data", 4) != 0) {
		printk("Unsupported WAV header layout\n");
		return -EINVAL;
	}

	if (hdr->audio_format != 1) {
		printk("Only PCM WAV supported\n");
		return -EINVAL;
	}

	if (hdr->bits_per_sample != 16) {
		printk("Only 16-bit WAV supported\n");
		return -EINVAL;
	}

	if (hdr->num_channels != 1 && hdr->num_channels != 2) {
		printk("Only mono or stereo WAV supported\n");
		return -EINVAL;
	}

	return 0;
}

static void apply_volume_to_stereo_block(int16_t *samples, size_t sample_count)
{
	for (size_t i = 0; i < sample_count; i++) {
		samples[i] = (int16_t)(((int32_t)samples[i] * g_volume) / 100);
	}
}

static ssize_t fill_tx_block_from_wav(struct fs_file_t *file,
				      void *tx_block,
				      size_t block_size,
				      const struct wav_header *hdr)
{
	if (hdr->num_channels == 2) {
		ssize_t n = fs_read(file, tx_block, block_size);

		if (n > 0) {
			size_t sample_count = (size_t)n / sizeof(int16_t);
			apply_volume_to_stereo_block((int16_t *)tx_block, sample_count);

			if (n < (ssize_t)block_size) {
				memset((uint8_t *)tx_block + n, 0,
				       block_size - (size_t)n);
			}
		}

		return n;
	}

	/* Mono 16-bit PCM -> duplicate into stereo. */
	int16_t *out = (int16_t *)tx_block;
	size_t stereo_frames = block_size / 4;
	size_t mono_bytes = stereo_frames * sizeof(int16_t);
	int16_t mono_buf[BLOCK_SIZE / 4];
	ssize_t n = fs_read(file, mono_buf, mono_bytes);

	if (n <= 0) {
		return n;
	}

	size_t samples_read = (size_t)n / sizeof(int16_t);

	for (size_t i = 0; i < samples_read; i++) {
		int16_t scaled = (int16_t)(((int32_t)mono_buf[i] * g_volume) / 100);

		out[2 * i] = scaled;
		out[2 * i + 1] = scaled;
	}

	for (size_t i = samples_read; i < stereo_frames; i++) {
		out[2 * i] = 0;
		out[2 * i + 1] = 0;
	}

	return n;
}

static int configure_i2s(const struct wav_header *hdr)
{
	struct i2s_config i2s_cfg = {
		.word_size = 16U,
		.channels = 2U,
		.format = I2S_FMT_DATA_FORMAT_I2S,
		.frame_clk_freq = hdr->sample_rate,
		.block_size = BLOCK_SIZE,
		.timeout = 2000,
		.options = I2S_OPT_FRAME_CLK_MASTER | I2S_OPT_BIT_CLK_MASTER,
		.mem_slab = &tx_0_mem_slab,
	};

	return i2s_configure(g_i2s_dev, I2S_DIR_TX, &i2s_cfg);
}

int playback_init(void)
{
	g_i2s_dev = DEVICE_DT_GET(DT_ALIAS(i2s_tx));
	if (!device_is_ready(g_i2s_dev)) {
		printk("I2S device not ready\n");
		return -ENODEV;
	}

	printk("playback_init\n");
	return 0;
}

int playback_play(const char *filename)
{
	struct fs_file_t file;
	struct wav_header hdr;
	char wav_path[sizeof(SD_MOUNT_POINT) + 1 + APP_MAX_FILENAME];
	size_t bytes_left;
	bool started = false;
	int ret;

	ret = mount_sd();
	if (ret < 0) {
		return ret;
	}

	if (filename == NULL || filename[0] == '\0') {
		return -EINVAL;
	}

	if (filename[0] == '/') {
		snprintk(wav_path, sizeof(wav_path), "%s", filename);
	} else {
		snprintk(wav_path, sizeof(wav_path), "%s/%s",
			 SD_MOUNT_POINT, filename);
	}

	printk("playback_play: %s\n", wav_path);

	fs_file_t_init(&file);

	ret = fs_open(&file, wav_path, FS_O_READ);
	if (ret < 0) {
		printk("fs_open(%s) failed: %d\n", wav_path, ret);
		return ret;
	}

	ret = read_wav_header(&file, &hdr);
	if (ret < 0) {
		fs_close(&file);
		return ret;
	}

	ret = configure_i2s(&hdr);
	if (ret < 0) {
		printk("Failed to configure I2S stream: %d\n", ret);
		fs_close(&file);
		return ret;
	}

	bytes_left = hdr.data_size;

	while (bytes_left > 0) {
		void *tx_block;
		ssize_t n;

		ret = k_mem_slab_alloc(&tx_0_mem_slab, &tx_block, K_FOREVER);
		if (ret < 0) {
			printk("Failed to allocate TX block\n");
			break;
		}

		n = fill_tx_block_from_wav(&file, tx_block, BLOCK_SIZE, &hdr);
		if (n <= 0) {
			printk("Read/conversion error: %d\n", (int)n);
			k_mem_slab_free(&tx_0_mem_slab, tx_block);
			ret = (n == 0) ? -EIO : (int)n;
			break;
		}

		ret = i2s_write(g_i2s_dev, tx_block, BLOCK_SIZE);
		if (ret < 0) {
			printk("i2s_write failed: %d\n", ret);
			k_mem_slab_free(&tx_0_mem_slab, tx_block);
			break;
		}

		if (!started) {
			ret = i2s_trigger(g_i2s_dev, I2S_DIR_TX, I2S_TRIGGER_START);
			if (ret < 0) {
				printk("Could not start I2S TX: %d\n", ret);
				break;
			}
			started = true;
		}

		bytes_left -= MIN(bytes_left, (size_t)n);
	}

	if (started) {
		int drain_ret = i2s_trigger(g_i2s_dev, I2S_DIR_TX, I2S_TRIGGER_DRAIN);

		if (drain_ret < 0) {
			printk("Could not drain I2S TX: %d\n", drain_ret);
			if (ret == 0) {
				ret = drain_ret;
			}
		}
	}

	fs_close(&file);
	return ret;
}

int playback_stop(void)
{
	int ret = i2s_trigger(g_i2s_dev, I2S_DIR_TX, I2S_TRIGGER_DROP);

	if (ret < 0) {
		printk("playback_stop failed: %d\n", ret);
		return ret;
	}

	printk("playback_stop\n");
	return 0;
}

int playback_pause(void)
{
	printk("playback_pause not supported by this I2S driver/API\n");
	return -ENOTSUP;
}

int playback_resume(void)
{
	printk("playback_resume not supported by this I2S driver/API\n");
	return -ENOTSUP;
}

int playback_set_volume(uint8_t volume)
{
	if (volume > 100) {
		volume = 100;
	}

	g_volume = volume;
	printk("playback_set_volume: %u\n", g_volume);
	return 0;
}
