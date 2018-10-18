-- hitbox visualizer for lizard

require "temp/lizard_addr"

-- box collectors

box_x0 = {};
box_y0 = {};
box_x1 = {};
box_y1 = {};
box_c  = {};
box_count = 0;
last_bank = 0xF;

function lizard_overlap()
	box_count = box_count + 1;
	box_x0[box_count] = memory.readbyte(addr_i+0) + (memory.readbyte(addr_ih+0) * 256);
	box_y0[box_count] = memory.readbyte(addr_i+1);
	box_x1[box_count] = memory.readbyte(addr_i+2) + (memory.readbyte(addr_ih+2) * 256);
	box_y1[box_count] = memory.readbyte(addr_i+3);
	box_c[box_count] = "#FF0000";
end;

function lizard_touch()
	box_count = box_count + 1;
	box_x0[box_count] = memory.readbyte(addr_i+0) + (memory.readbyte(addr_ih+0) * 256);
	box_y0[box_count] = memory.readbyte(addr_i+1);
	box_x1[box_count] = memory.readbyte(addr_i+2) + (memory.readbyte(addr_ih+2) * 256);
	box_y1[box_count] = memory.readbyte(addr_i+3);
	box_c[box_count] = "#0000FF";
end;

function lizard_box()
	box_count = box_count + 1;
	box_x0[box_count] = memory.readbyte(addr_lizard_hitbox_x0) + (memory.readbyte(addr_lizard_hitbox_xh0) * 256);
	box_y0[box_count] = memory.readbyte(addr_lizard_hitbox_y0);
	box_x1[box_count] = memory.readbyte(addr_lizard_hitbox_x1) + (memory.readbyte(addr_lizard_hitbox_xh1) * 256);
	box_y1[box_count] = memory.readbyte(addr_lizard_hitbox_y1);
	box_c[box_count] = "#00FF00";
	box_count = box_count + 1;
	box_x0[box_count] = memory.readbyte(addr_lizard_touch_x0) + (memory.readbyte(addr_lizard_touch_xh0) * 256);
	box_y0[box_count] = memory.readbyte(addr_lizard_touch_y0);
	box_x1[box_count] = memory.readbyte(addr_lizard_touch_x1) + (memory.readbyte(addr_lizard_touch_xh1) * 256);
	box_y1[box_count] = memory.readbyte(addr_lizard_touch_y1);
	box_c[box_count] = "#00FF00";
end;

-- CONTINUE FROM HERE

function blocker_boxes()
	for i=1,4 do
		box_count = box_count + 1;
		box_x0[box_count] = memory.readbyte(addr_blocker_x0+i-1) + (memory.readbyte(addr_blocker_xh0+i-1) * 256);
		box_y0[box_count] = memory.readbyte(addr_blocker_y0+i-1);
		box_x1[box_count] = memory.readbyte(addr_blocker_x1+i-1) + (memory.readbyte(addr_blocker_xh1+i-1) * 256);
		box_y1[box_count] = memory.readbyte(addr_blocker_y1+i-1);
		box_c[box_count] = "#FFFF00";
	end;
end;

function lizard_overlapF()
	if last_bank == 0xF then
		lizard_overlap()
	end
end;

function lizard_overlapE()
	if last_bank == 0xE then
		lizard_overlap()
	end
end;

function lizard_overlapD()
	if last_bank == 0xD then
		lizard_overlap()
	end
end;

function lizard_touchF()
	if last_bank == 0xF then
		lizard_touch()
	end
end;

function lizard_touchE()
	if last_bank == 0xE then
		lizard_touch()
	end
end;

function lizard_touchD()
	if last_bank == 0xD then
		lizard_touch()
	end
end;

function bank_swap()
	last_bank = memory.getregister("a")
end;

-- draw the boxes

function draw()
	lizard_box()
	blocker_boxes()
	local scroll_x = 0
	local dog_type0 = memory.readbyte(addr_dog_type+0)
	if dog_type0 ~= addr_DOG_RIVER and dog_type0 ~= addr_DOG_FROB then
		scroll_x = memory.readbyte(addr_scroll_x+0) + (memory.readbyte(addr_scroll_x+1) * 256);
	end
	for i=1,box_count do
		local x0 = box_x0[i] - scroll_x;
		local y0 = box_y0[i];
		local x1 = box_x1[i] - scroll_x;
		local y1 = box_y1[i];
		local c  = box_c[ i];
		-- reverse boxes over the edge do not test true for overlap, but it is a good diagnostic to see them
		--if (x0 <= x1) and (y0 <= y1) then
		-- boxes at 65535,255 are intentionally hidden
		if x0 ~= 65535 or x1 ~= 65535 or y0 ~= 255 or y1 ~= 255 then
			gui.box(x0,y0,x1,y1,"",c)
		end
		--end;
	end;
end;

-- main

--gui.register(draw);
emu.registerafter(draw);
--memory.registerexec(addr_lizard_overlapF,lizard_overlapF);
memory.registerexec(addr_lizard_overlapE,lizard_overlapE);
memory.registerexec(addr_lizard_overlapD,lizard_overlapD);
--memory.registerexec(addr_lizard_touchF  ,lizard_touchF  );
memory.registerexec(addr_lizard_touchE  ,lizard_touchE  );
memory.registerexec(addr_lizard_touchD  ,lizard_touchD  );
memory.registerwrite(addr_bus_conflict,0x10,bank_swap);

while (true) do
	box_count = 0;
	emu.frameadvance();
end;

-- end of file
