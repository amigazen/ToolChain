EXECS = append args date fahr hello null sizeofs Pi Savage Sieve

all:	$(EXECS)

append: append.c
	ccx append.c

args:	args.c
	ccx args.c

date:	date.c
	ccx date.c

fahr:	fahr.c
	ccx -lmath fahr.c

hello:	hello.c
	ccx hello.c

null:	null.c
	ccx null.c

sizeofs: sizeofs.c
	ccx sizeofs.c

Pi:	Pi.c
	ccx -lmath Pi.c

Savage:	Savage.c
	ccx -lmath Savage.c

Sieve:	Sieve.c
	ccx Sieve.c

clean:
	delete (append|args|date|fahr|hello|null|sizeofs|Pi|Savage|Sieve)

