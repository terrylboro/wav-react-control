#include <stdbool.h>
#include <stdint.h>
#include <string.h>

#include <zephyr/kernel.h>
#include <zephyr/sys/printk.h>

#include "app_cmd.h"
#include "ble_control.h"
#include "playback.h"

#define PLAYER_STACK_SIZE 4096
#define PLAYER_PRIORITY 5

K_MSGQ_DEFINE(player_cmd_q, sizeof(struct app_cmd), 8, 4);

struct player_state {
	bool playing;
	bool paused;
	uint8_t volume;
	char current_file[APP_MAX_FILENAME];
};

static struct player_state g_player = {
	.playing = false,
	.paused = false,
	.volume = 100,
	.current_file = "SAMPL~16.WAV",
};

K_THREAD_STACK_DEFINE(player_stack, PLAYER_STACK_SIZE);
static struct k_thread player_thread_data;

static void handle_cmd(const struct app_cmd *cmd)
{
	int ret;

	switch (cmd->type) {
	case APP_CMD_PLAY:
		strncpy(g_player.current_file, cmd->filename,
			sizeof(g_player.current_file) - 1);
		g_player.current_file[sizeof(g_player.current_file) - 1] = '\0';

		ret = playback_play(g_player.current_file);
		if (ret == 0) {
			g_player.playing = true;
			g_player.paused = false;
		} else {
			printk("PLAY failed: %d\n", ret);
			g_player.playing = false;
			g_player.paused = false;
		}
		break;

	case APP_CMD_STOP:
		ret = playback_stop();
		if (ret < 0) {
			printk("STOP failed: %d\n", ret);
		}
		g_player.playing = false;
		g_player.paused = false;
		break;

	case APP_CMD_PAUSE:
		if (g_player.playing && !g_player.paused) {
			ret = playback_pause();
			if (ret < 0) {
				printk("PAUSE failed: %d\n", ret);
			} else {
				g_player.paused = true;
			}
		}
		break;

	case APP_CMD_RESUME:
		if (g_player.playing && g_player.paused) {
			ret = playback_resume();
			if (ret < 0) {
				printk("RESUME failed: %d\n", ret);
			} else {
				g_player.paused = false;
			}
		}
		break;

	case APP_CMD_SET_VOLUME:
		ret = playback_set_volume(cmd->volume);
		if (ret < 0) {
			printk("SET_VOLUME failed: %d\n", ret);
		} else {
			g_player.volume = cmd->volume;
		}
		break;

	default:
		printk("Unhandled command type: %d\n", cmd->type);
		break;
	}
}

static void player_thread(void *p1, void *p2, void *p3)
{
	ARG_UNUSED(p1);
	ARG_UNUSED(p2);
	ARG_UNUSED(p3);

	struct app_cmd cmd;

	printk("Playback thread started\n");

	for (;;) {
		if (k_msgq_get(&player_cmd_q, &cmd, K_FOREVER) == 0) {
			handle_cmd(&cmd);
		}
	}
}

int main(void)
{
	int err;

	printk("Starting BLE WAV controller\n");

	err = playback_init();
	if (err) {
		printk("Playback init failed: %d\n", err);
		return err;
	}

	k_thread_create(&player_thread_data,
			player_stack,
			K_THREAD_STACK_SIZEOF(player_stack),
			player_thread,
			NULL, NULL, NULL,
			PLAYER_PRIORITY,
			0,
			K_NO_WAIT);

	err = ble_control_init(&player_cmd_q);
	if (err) {
		printk("BLE control init failed: %d\n", err);
		return err;
	}

	printk("Ready\n");
	return 0;
}
