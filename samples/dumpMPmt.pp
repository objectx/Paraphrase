"mersenne" : 2LL swap pow 1- ;

"lucas-lehmer-test-fast" :
	dup 1 over mersenne	rot	2 - rot swap 4LL inv-rot
	for+
		dup * dup 4 pick >>	swap 3 pick	& +	dup	3 pick	
		>= if over - then
		2 -	
	next
	0 == inv-rot drop drop
;

"prime?" :
	switch
		case 1 ==	-> false break
		case 2 ==	-> true  break
		case 2 % 0?	-> false break 
		// otherwise
	    true swap dup sqrt ceil >int 3 swap
		for+ dup i % 0? if swap drop false swap leave then 2 step next
    	drop
	dispatch
;

"num-of-digit" :
	dup log10 floor >int 1+ dup 3 +
	for+ dup 10 >INT i >int pow < if drop i 1- leave then next
	1+
;

// n ---
"calc-and-print-mersenne" :
	dup "p=%5d, " putf
	mersenne dup num-of-digit dup 13 > if
		dup inv-rot
		5 - 10 >INT swap pow swap dup rot / write	
		"..." write 10 5 pow % "%05d" putf " (%5d digits)" putf
	else
		drop "%13d" putf
	then
;

// check 1...4 by single-thread
() 1 4 for+
	i prime? if
		i mersenne prime? if i append then
	then
next

// check 4...5000 by multi-thread
reset-pipes
[ 5 5000 for+ i >pipe 2 step next ]
[[
	while-pipe
		dup prime? if
			dup lucas-lehmer-test-fast if >pipe then
		then
	repeat
]]
while-pipe append repeat { < } sort
0 >r {
	r> 1+ dup "No=%2d, " putf >r
	calc-and-print-mersenne cr
} apply
