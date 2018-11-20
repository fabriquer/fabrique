function dag
	ninja -C build everything ; \
	and ./build/bin/fab --format=null --print-ast --print-dag $argv
end
