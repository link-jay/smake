run:bar.txt baz.txt:Create foo.txt
	touch foo.txt

bar.txt::Create bar.txt
	touch bar.txt

baz.txt::Create baz.txt
	touch baz.txt

clean::clean all.
	echo clean
