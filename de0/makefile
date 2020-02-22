all:
	iverilog -g2005-sv -DICARUS=1 avr.v cpu.v alu.v spi.v sdram.v -o main.qqq
	vvp main.qqq >> /dev/null

wave:
	gtkwave avr.gtkw

mif:
	date
	quartus_cdb de0 -c de0 --update_mif
	quartus_asm --read_settings_files=on --write_settings_files=off de0 -c de0
	date

avr:
	./../av2/avr avr.bin

clean:
	rm -f main.qqq
	rm -rf db incremental_db simulation timing
	rm -f *.jdi *.pof *.sld *.rpt *.summary *.sof *.done *.pin *.qws *.bak *.smsg *.qws *.vcd *.qqq *.jic *.map *.qqq
	rm -f greybox_tmp undo_redo.txt PLLJ_PLLSPE_INFO.txt c5_pin_model_dump.txt
	rm -rf .qsys_edit *.o *.bin *.elf

