function debug
	ninja -C build everything ; \
	and lldb -- ./build/bin/fab --format=null --print-ast --print-dag $argv
end
