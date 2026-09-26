# OpenOCD via Raspberry Pi Debug Probe (CMSIS-DAP).
# Pre-init commands written out here (instead of openocd-nrf5.board.cmake)
# so the 100 kHz speed comes AFTER the target script, which sets 1 MHz.
board_runner_args(openocd --cmd-pre-init "set WORKAREASIZE 0x4000")
board_runner_args(openocd --cmd-pre-init "source [find interface/cmsis-dap.cfg]")
board_runner_args(openocd --cmd-pre-init "transport select swd")
board_runner_args(openocd --cmd-pre-init "source [find target/nrf52.cfg]")
board_runner_args(openocd --cmd-pre-init "adapter speed 100")
include(${ZEPHYR_BASE}/boards/common/openocd.board.cmake)

# Alternatives if a J-Link is added later
board_runner_args(jlink "--device=nRF52840_xxAA" "--speed=4000")
board_runner_args(pyocd "--target=nrf52840" "--frequency=4000000")
include(${ZEPHYR_BASE}/boards/common/nrfutil.board.cmake)
include(${ZEPHYR_BASE}/boards/common/nrfjprog.board.cmake)
include(${ZEPHYR_BASE}/boards/common/jlink.board.cmake)
include(${ZEPHYR_BASE}/boards/common/pyocd.board.cmake)