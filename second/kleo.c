/*	kleo.c

	Kentucky's Line Extrusion Orderer

	2019 by H. Dietz.
*/

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <time.h>
#include "omp.h"

#define	MAXPLACE	(1024)
#define	MAXPOP		(8*1024)
#define	MAXPRIO		0x8000
#define	FLIP(PRIO)	((PRIO)^MAXPRIO)
#define	ISFLIP(PRIO)	((PRIO)>=MAXPRIO)
#define	UNFLIP(PRIO)	((PRIO)&(MAXPRIO-1))
#define	RANDPRIO	(rand() & (MAXPRIO + (MAXPRIO-1)))	
#define	RANDPLACE	(rand() % places)

#define	forplaces(I)	for (I=0; I<places; ++I)
#define	SWAP(X,Y)	do { X^=Y; Y^=X; X^=Y; } while(0)

float	lines[MAXPLACE][4];
int	p[MAXPOP][MAXPLACE];
float	m[MAXPOP];
int	places;

typedef	struct {
	int	line;
	int	prio;
} lineprio_t;

float	startx, starty;

extern	float sqrtf(float x);

int
cmplp(const void *a,
const void *b)
{
	int aprio = UNFLIP(((lineprio_t *) a)->prio);
	int bprio = UNFLIP(((lineprio_t *) b)->prio);

	return(aprio - bprio);
}

void
mkorder(register lineprio_t *order, register int who)
{
	register int i;

	forplaces(i) {
		order[i].line = i;
		order[i].prio = p[who][i];
	}
	qsort(order, places, sizeof(lineprio_t), cmplp);
}

void
showorder(register FILE *fp, register lineprio_t *order)
{
	/* Show lines in the current order */
	register int i;

	forplaces(i) {
		register int j = order[i].line;
		register int k = (ISFLIP(order[i].prio) ? 2 : 0);

		fprintf(fp,
			"%f %f %f %f\n",
			lines[j][k],
			lines[j][k^1],
			lines[j][k^2],
			lines[j][k^3]);
	}

	/* Force output flush */
	fflush(fp);
}

inline static float
dist(register float dx,
register float dy)
{
	return(sqrtf((dx * dx) + (dy * dy)));
}

float
costorder(register lineprio_t *order)
{
	/* Compute cost in the current order */
	register float d = 0;
	register float x = startx;
	register float y = starty;
	register int i;

	forplaces(i) {
		register int j = order[i].line;
		register int k = (ISFLIP(order[i].prio) ? 2 : 0);

		j = UNFLIP(j);
		d += dist((x - lines[j][k]),
			  (y - lines[j][k^1]));
		d += dist((lines[j][0] - lines[j][2]),
			  (lines[j][1] - lines[j][3]));
		x = lines[j][k^2];
		y = lines[j][k^3];
	}

	return(d);
}
		
#ifdef	VERBOSE
char	*by;
#endif

void
asis(register int a)
{
	/* a = greedy path */
	register int i;

	/* zero everything */
	forplaces(i) {
		p[a][i] = ((i * (MAXPRIO / places)) +
			   (rand() % (MAXPRIO / places)));
	}

#ifdef	VERBOSE
	by = "Original";
#endif
}

void
greedy(register int a, register int r)
{
	/* a = greedy path from (r ? random : start) */
	register int i;
	register float x = startx;
	register float y = starty;

	if (r) {
		register int place = RANDPLACE;
		register int pend = (rand() & 2);
		x = lines[place][pend];
		y = lines[place][pend+1];
	}

	/* zero everything */
	forplaces(i) {
		p[a][i] = -1;
	}

	/* pick nearest next */
	forplaces(i) {
		register float bestd = 1000000;
		register int best = 0;
		register int flip = 0;
		register int j;

		forplaces(j) if (p[a][j] == -1) {
			register float d = dist((x - lines[j][0]),
						(y - lines[j][1]));
			if (d < bestd) {
				bestd = d;
				best = j;
				flip = 0;
			}

			d = dist((x - lines[j][2]),
				 (y - lines[j][3]));
			if (d < bestd) {
				bestd = d;
				best = j;
				flip = 2;
			}
		}

		p[a][best] = ((i * (MAXPRIO / places)) +
			      (rand() % (MAXPRIO / places)));
		if (flip) {
			p[a][best] |= MAXPRIO;
		}
		x = lines[best][flip^2];
		y = lines[best][flip^3];
	}

#ifdef	VERBOSE
	by = "Greedy";
#endif
}

int
flipper(register int a,
register int b)
{
	/* a = a with lines flipped where it helps */
	register int i;
	register float x = startx;
	register float y = starty;
	register int flipped = 0;
	lineprio_t order[MAXPLACE];

	/* Copy b */
	forplaces(i) {
		p[a][i] = p[b][i];
	}

	/* Make the order */
	mkorder(order, a);

	/* For each, flip it if appropriate */
	forplaces(i) {
		register int j = order[i].line;
		register int k = order[i].prio;
		register float d = dist((x - lines[j][0]),
					(y - lines[j][1]));
		register float dflip = dist((x - lines[j][2]),
					    (y - lines[j][3]));

		p[a][j] = ((d < dflip) ?
			   (p[a][j] & ~MAXPRIO) :
			   (p[a][j] | MAXPRIO));
		x = lines[j][(d < dflip) ? 2 : 0];
		y = lines[j][(d < dflip) ? 3 : 1];
		flipped += (p[a][j] != order[i].prio);
	}


#ifdef	VERBOSE
	if (flipped > 0) by = "Flipper";
#endif
	return(flipped);
}

void
rotate(register int a,
register int b)
{
	/* a = rotate(b) */
	register int i;
	register int j = (RANDPRIO & (MAXPRIO - 1));

	/* Rotate values by j */
	forplaces(i) {
		p[a][i] = ((p[a][i] & MAXPRIO) |
			   ((p[a][i] + j) & (MAXPRIO - 1)));
	}

#ifdef	VERBOSE
	by = "Rotate";
#endif
}

void
mutate(register int a,
register int b)
{
	/* a = mutate(b) */
	register int i;
	register int j = RANDPLACE;

	/* Stochastically copy b or random */
	forplaces(i) {
		p[a][i] = ((j != i) ? p[b][i] : RANDPRIO);
	}

#ifdef	VERBOSE
	by = "Mutate";
#endif
}

void
cross(register int a,
register int b,
register int c)
{
	/* a = cross(b, c) */
	register int i;

	forplaces(i) {
		p[a][i] = p[(rand() & 0x80) ? b : c][i];
	}

#ifdef	VERBOSE
	by = "Cross";
#endif
}

int
main(int argc, char **argv)
{
	int i, abest = -1;
	float m[MAXPOP], mbest;
	int pop, cro, mut, gens, gen = 0;
	float ref[MAXPLACE];
	int dumtest = 0;
	lineprio_t order[MAXPLACE];
	omp_set_num_threads(4);

	if (argc == 7) {
		pop = atoi(argv[1]);
		cro = atoi(argv[2]);
		mut = atoi(argv[3]);
		gens = atoi(argv[4]);
		startx = atof(argv[5]);
		starty = atof(argv[6]);
		if ((pop < 3) || (pop > MAXPOP)) {
			fprintf(stderr,
				"%s: population must be between 3 and %d\n",
				argv[0],
				MAXPOP);
			exit(2);
		}
	} else {
		fprintf(stderr,
			"Usage: %s [pop] [cro] [mut] [gens] [startx] [starty]\n",
			argv[0]);
		exit(1);
	}

	/* Read the problem from stdin */
	for (places=0;
	     ((places < MAXPLACE) &&
	      (scanf("%f%f%f%f",
		     &(lines[places][0]),
		     &(lines[places][1]),
		     &(lines[places][2]),
		     &(lines[places][3])) == 4));
	     ++places) ;

	/* Initialize random generator */
	srand(time(0));

	/* Create initial population */
	do {
		register int a;

		/* Make a new guy... */
		if (gen < 1) {
			asis(a = gen);
		} else if (gen < 2) {
			greedy(a = gen, 0);
		} else if (rand() & 1024) {
			greedy(a = gen, 1);
		} else {
			a = gen;
			forplaces(i) {
				p[a][i] = RANDPRIO;
			}
#ifdef	VERBOSE
			by = "Random";
#endif
		}

		/* Evaluate the metric & update best */
		mkorder(order, a);
		m[a] = costorder(order);
		if ((gen == 0) || (m[a] < mbest)) {
			mbest = m[a];
			abest = a;

			/* Print the stats */
#ifdef	VERBOSE
			fprintf(stderr,
				"Gen %d, by %s, %f:\n",
				gen,
				by,
				mbest);
			showorder(stderr, order);
			fprintf(stderr,
				"\n");
#endif
		}
	} while (++gen < pop);

	/* The genetic algorithm loop... */
#pragma omp parallel
{
	#pragma omp for
	for (int i = gen; i < gens; i++) {
		//printf("i = %d, I am Thread %d\n", i, omp_get_thread_num());
		/* Pick a pair of random pop members */
		register int a, b;
		a = (rand() % pop);
		do {
			b = (rand() % pop);
		} while (a == b);

		/* Make a be the worst one */
		if ((m[a] < m[b]) || (a == abest)) {
			SWAP(a, b);
		}

		/* Crossover or mutation? */
		if ((rand() % (cro + mut)) < cro) {
			/* Crossover needs a third */
			register int c = b;

			do {
				b = (rand() % pop);
			} while ((a == b) || (c == b));

			/* Make a be the worst one */
			if ((m[a] < m[b]) || (a == abest)) {
				SWAP(a, b);
			}

			/* Crossover */
			cross(a, b, c);
		} else {
			/* Mutation */
			if ((rand() & 0x601) ||
			    (flipper(a, b) < 1)) {
				if (rand() & 0x4000) {
					mutate(a, b);
				} else {
					rotate(a, b);
				}
			}
		}

		/* Evaluate the metric & update best */
		mkorder(order, a);
#pragma omp critical
		m[a] = costorder(order);
		if ((gen == 0) || (m[a] < mbest)) {
			mbest = m[a];
			abest = a;

			/* Print the stats */
#ifdef	VERBOSE
			fprintf(stderr,
				"Gen %d, by %s, %f:\n",
				i,
				by,
				mbest);
			showorder(stderr, order);
			fprintf(stderr,
				"\n");
#endif
		}
	}
}
	printf("Best is %f:\n",
	       m[abest]);
	mkorder(order, abest);
	showorder(stdout, order);
	printf("\n");
	exit(0);
}
