-- clock display for lizard

require "temp/lizard_addr"

while (true) do
	emu.frameadvance();
	th = memory.readbyte(addr_metric_time_h)
	tm = memory.readbyte(addr_metric_time_m)
	ts = memory.readbyte(addr_metric_time_s)
	tf = memory.readbyte(addr_metric_time_f)
	gui.text(8,8,string.format("%02d:%02d:%02d/%02d",th,tm,ts,tf),"#FFFF00","#00000040")
end;

-- end of file
