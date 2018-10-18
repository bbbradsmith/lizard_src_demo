-- simple sprite visualizer for FCEUX

sprite_height = 8        -- change to 16 for a game using double-height sprites
sprite_color = "#FF00FF" -- change to customize color
oam_dma_source = "a"     -- change to "x" or "y" if OAM DMA register ($4014) is not written from A

function oam_dma()
	local oam = memory.getregister(oam_dma_source) * 256
	for i=1,64 do
		local is = (i-1) * 4
		local x0 = memory.readbyte(oam + is + 3)
		local x1 = x0 + 7
		local y0 = memory.readbyte(oam + is + 0) + 1
		local y1 = y0 + (sprite_height - 1)
		gui.box(x0,y0,x1,y1,"",sprite_color)
	end
end

-- main

memory.registerwrite(0x4014,1,oam_dma)

while (true) do
	emu.frameadvance()
end
