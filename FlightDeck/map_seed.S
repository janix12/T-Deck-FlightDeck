.section .rodata.flightdeck_map,"a"
.balign 4
.global flightdeck_map_start
.global flightdeck_map_end
flightdeck_map_start:
.incbin "hungary.fdm"
flightdeck_map_end:
.balign 4
.global flightdeck_overview_start
flightdeck_overview_start:
.incbin "central-europe.fdm"
