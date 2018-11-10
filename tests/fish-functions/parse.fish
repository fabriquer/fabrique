function parse
	ninja -C build everything ; \
	and ./build/bin/fab --debug='*' --parse-only --print-ast $argv
end
