foo.txt:bar.txt baz.txt test:Create foo.txt
	touch foo.txt

bar.txt::Create bar.txt
	touch bar.txt

baz.txt::Create baz.txt
	touch baz.txt

test::just test
	true
