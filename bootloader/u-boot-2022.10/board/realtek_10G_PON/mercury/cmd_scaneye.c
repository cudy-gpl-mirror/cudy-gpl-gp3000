
#include <common.h>
#include <command.h>
#include <env.h>
#include <asm/io.h>

static volatile uint32_t *reg_base, *reg_read, *reg_data;

static char decode_data(uint32_t data)
{
	if ((data & (3 << 14)) != (3 << 14)) /* b[15:14] should be b'11 */
		return 'X'; /* invalid */

	if (data & 0x2AAA)
		return '1'; /* nok */
	else
		return '0'; /* ok */
}

static int do_scaneye(struct cmd_tbl *cmdtp, int flag, int argc,
		      char *const argv[])
{
	const char *cmd = argc < 2 ? NULL : argv[1];
	volatile uint32_t val_base, val_read, val_data;
	uint8_t i, x, y;
	char ch;

	if (!cmd)
		return cmd_usage(cmdtp);

	switch (*cmd) {
	case '0':
		reg_base = (uint32_t *)0xf47041a8;
		reg_read = (uint32_t *)0xf47041b8;
		reg_data = (uint32_t *)0xf4706018;
		break;

	case '1':
		reg_base = (uint32_t *)0xf47141a8;
		reg_read = (uint32_t *)0xf47141b8;
		reg_data = (uint32_t *)0xf4716018;
		break;

	default:
		printf("Invalid Channel %c\n", *cmd);
		return 0;
	}

	printf("v00 base=%p read=%p data=%p\n", reg_base, reg_read, reg_data);

	val_base = (*reg_base) & ~0xfff;

	printf("   :0         10        20        30        40        50        60\n");
	printf("   :0123456789012345678901234567890123456789012345678901234567890123\n");

	for (y = 0; y < 64; y++) {
		printf("%3d ", y);

		for (x = 0; x < 64; x++) {
			*reg_base = (val_base) | (y << 6) | (x);
			val_read = *reg_read;
			val_read &= ~BIT(0);

			/*toggle*/
			for (i = 0; i < 3; i++) {
				*reg_read = (val_read);
				mb();
				*reg_read = (val_read | BIT(0));
				mb();
			}

			val_data = *reg_data;

			ch = decode_data(val_data);
			printf("%c", ch);

			if (ch == 'X') {
				printf("\nInvlid ouput, stopped\n");
				return 0;
			}

#if 0

			/* for debug */
			if (x == 63)
				printf(" dbg %x,%x", val_base, val_read);

#endif
		}

		printf("\n");
	}

	return 0;
}

U_BOOT_CMD(
	scaneye,  2,      0,      do_scaneye,
	"Serdes Scan Eye",
	"<0..6>"
);
