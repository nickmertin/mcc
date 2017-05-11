	.file "hello-test.cre"
	.text
	.globl is_hello
is_hello:
	pushl %ebx
	subl $7, %esp
	movl 15(%esp), %eax
	movl %eax, 0(%esp)
	# byte $1 @ 4
	# byte $2 @ 5
	# $1 = *$0
	movl 0(%esp), %eax
	movb 0(%eax), %al
	movb %al, 4(%esp)
	# if $1 goto _0
	cmpb $0, 4(%esp)
	jne _is_hello$$_0
	# byte $3 @ 6
	# $3 = 0
	movb $0, 6(%esp)
	# return $3
	xorl %eax, %eax
	movb 6(%esp), %al
	jmp _is_hello$end
	# delete $3
	# _0:
_is_hello$$_0:
	# $2 = 104
	movb $104, 5(%esp)
	# $2 = $2 == $1
	xorl %eax, %eax
	movb 5(%esp), %al
	xorl %ebx, %ebx
	movb 4(%esp), %bl
	cmpl %eax, %ebx
	jne _is_hello$0
	movl $1, %eax
	jmp _is_hello$1
_is_hello$0:
	xorl %eax, %eax
_is_hello$1:
	movb %al, 5(%esp)
	# if $2 goto _1
	cmpb $0, 5(%esp)
	jne _is_hello$$_1
	# delete $1
	# delete $2
	# byte $4 @ 4
	# $4 = 0
	movb $0, 4(%esp)
	# return $4
	xorl %eax, %eax
	movb 4(%esp), %al
	jmp _is_hello$end
	# delete $4
	# _1:
_is_hello$$_1:
	# $0 = ++$0
	incl 0(%esp)
	# byte $5 @ 4
	# byte $6 @ 5
	# $5 = *$0
	movl 0(%esp), %eax
	movb 0(%eax), %al
	movb %al, 4(%esp)
	# if $5 goto _2
	cmpb $0, 4(%esp)
	jne _is_hello$$_2
	# byte $7 @ 6
	# $7 = 0
	movb $0, 6(%esp)
	# return $7
	xorl %eax, %eax
	movb 6(%esp), %al
	jmp _is_hello$end
	# delete $7
	# _2:
_is_hello$$_2:
	# $6 = 101
	movb $101, 5(%esp)
	# $6 = $6 == $5
	xorl %eax, %eax
	movb 5(%esp), %al
	xorl %ebx, %ebx
	movb 4(%esp), %bl
	cmpl %eax, %ebx
	jne _is_hello$2
	movl $1, %eax
	jmp _is_hello$3
_is_hello$2:
	xorl %eax, %eax
_is_hello$3:
	movb %al, 5(%esp)
	# if $6 goto _3
	cmpb $0, 5(%esp)
	jne _is_hello$$_3
	# delete $5
	# delete $6
	# byte $8 @ 4
	# $8 = 0
	movb $0, 4(%esp)
	# return $8
	xorl %eax, %eax
	movb 4(%esp), %al
	jmp _is_hello$end
	# delete $8
	# _3:
_is_hello$$_3:
	# $0 = ++$0
	incl 0(%esp)
	# byte $9 @ 4
	# byte $10 @ 5
	# $9 = *$0
	movl 0(%esp), %eax
	movb 0(%eax), %al
	movb %al, 4(%esp)
	# if $9 goto _4
	cmpb $0, 4(%esp)
	jne _is_hello$$_4
	# byte $11 @ 6
	# $11 = 0
	movb $0, 6(%esp)
	# return $11
	xorl %eax, %eax
	movb 6(%esp), %al
	jmp _is_hello$end
	# delete $11
	# _4:
_is_hello$$_4:
	# $10 = 108
	movb $108, 5(%esp)
	# $10 = $10 == $9
	xorl %eax, %eax
	movb 5(%esp), %al
	xorl %ebx, %ebx
	movb 4(%esp), %bl
	cmpl %eax, %ebx
	jne _is_hello$4
	movl $1, %eax
	jmp _is_hello$5
_is_hello$4:
	xorl %eax, %eax
_is_hello$5:
	movb %al, 5(%esp)
	# if $10 goto _5
	cmpb $0, 5(%esp)
	jne _is_hello$$_5
	# delete $9
	# delete $10
	# byte $12 @ 4
	# $12 = 0
	movb $0, 4(%esp)
	# return $12
	xorl %eax, %eax
	movb 4(%esp), %al
	jmp _is_hello$end
	# delete $12
	# _5:
_is_hello$$_5:
	# $0 = ++$0
	incl 0(%esp)
	# byte $13 @ 4
	# byte $14 @ 5
	# $13 = *$0
	movl 0(%esp), %eax
	movb 0(%eax), %al
	movb %al, 4(%esp)
	# if $13 goto _6
	cmpb $0, 4(%esp)
	jne _is_hello$$_6
	# byte $15 @ 6
	# $15 = 0
	movb $0, 6(%esp)
	# return $15
	xorl %eax, %eax
	movb 6(%esp), %al
	jmp _is_hello$end
	# delete $15
	# _6:
_is_hello$$_6:
	# $14 = 108
	movb $108, 5(%esp)
	# $14 = $14 == $13
	xorl %eax, %eax
	movb 5(%esp), %al
	xorl %ebx, %ebx
	movb 4(%esp), %bl
	cmpl %eax, %ebx
	jne _is_hello$6
	movl $1, %eax
	jmp _is_hello$7
_is_hello$6:
	xorl %eax, %eax
_is_hello$7:
	movb %al, 5(%esp)
	# if $14 goto _7
	cmpb $0, 5(%esp)
	jne _is_hello$$_7
	# delete $13
	# delete $14
	# byte $16 @ 4
	# $16 = 0
	movb $0, 4(%esp)
	# return $16
	xorl %eax, %eax
	movb 4(%esp), %al
	jmp _is_hello$end
	# delete $16
	# _7:
_is_hello$$_7:
	# $0 = ++$0
	incl 0(%esp)
	# byte $17 @ 4
	# byte $18 @ 5
	# $17 = *$0
	movl 0(%esp), %eax
	movb 0(%eax), %al
	movb %al, 4(%esp)
	# if $17 goto _8
	cmpb $0, 4(%esp)
	jne _is_hello$$_8
	# byte $19 @ 6
	# $19 = 0
	movb $0, 6(%esp)
	# return $19
	xorl %eax, %eax
	movb 6(%esp), %al
	jmp _is_hello$end
	# delete $19
	# _8:
_is_hello$$_8:
	# $18 = 111
	movb $111, 5(%esp)
	# $18 = $18 == $17
	xorl %eax, %eax
	movb 5(%esp), %al
	xorl %ebx, %ebx
	movb 4(%esp), %bl
	cmpl %eax, %ebx
	jne _is_hello$8
	movl $1, %eax
	jmp _is_hello$9
_is_hello$8:
	xorl %eax, %eax
_is_hello$9:
	movb %al, 5(%esp)
	# if $18 goto _9
	cmpb $0, 5(%esp)
	jne _is_hello$$_9
	# delete $17
	# delete $18
	# byte $20 @ 4
	# $20 = 0
	movb $0, 4(%esp)
	# return $20
	xorl %eax, %eax
	movb 4(%esp), %al
	jmp _is_hello$end
	# delete $20
	# _9:
_is_hello$$_9:
	# $0 = ++$0
	incl 0(%esp)
	# byte $21 @ 4
	# $21 = 1
	movb $1, 4(%esp)
	# return $21
	xorl %eax, %eax
	movb 4(%esp), %al
	jmp _is_hello$end
	# delete $21
_is_hello$end:
	addl $7, %esp
	popl %ebx
	ret
