/* See LICENSE file for copyright and license details. */

#define MAX(A, B)               ((A <= B) * B + (A > B) * A)
#define MIN(A, B)               ((A >= B) * B + (A < B) * A)
#define BETWEEN(X, A, B)        ((A) <= (X) && (X) <= (B))
#define MOD(N,M)                ((N) % (M) < 0) * ((N)%(M) +(M)) + !((N) % (M) < 0) * ((N) % (M)) 
#define TRUNC(X,A,B)            (MAX((A), MIN((X), (B))))

void die(const char *fmt, ...);
void *ecalloc(size_t nmemb, size_t size);
char *smprintf(char *fmt, ...);
void syslog(char *text);

