function check
	ninja -C build everything ; \
	and lit $argv
end
