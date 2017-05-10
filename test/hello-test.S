	.file "hello-test.cre"
	.text
	.globl is_hello
is_hello:
	pushl %ebx
	pushl %ebp
	movl %esp, %ebp
	subl $5, %esp
	movl 13(%esp), %eax
	movl %eax, 0(%esp)
	# byte $1 @ 4
	# $1 = 1
	movb $1, 4(%esp)
	# return $1
	xorl %eax, %eax
	movb 4(%esp), %al
	jmp _is_hello$end
	# delete $1
_is_hello$end:
	leave
	popl %ebx
	ret
