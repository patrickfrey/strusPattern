// https://stackoverflow.com/questions/35199079/fuzzy-regex-match-using-tre
// Example by Wiktor Stribi\u017cew 

#include <stdio.h>
#include "tre/tre.h"

int main() {
    regex_t rx;
    tre_regcomp(&rx, "(January|February)", REG_EXTENDED | REG_APPROX_MATCHER);

    regaparams_t params;
    params.cost_ins = 1;
    params.cost_del = 1;
    params.cost_subst = 1;
    params.max_cost = 2;
    params.max_del = 2;
    params.max_ins = 2;
    params.max_subst = 2;
    params.max_err = 2;

    regamatch_t match;
    match.nmatch = 0;
    match.pmatch = 0;

    if (!tre_regaexec(&rx, "Janvary", &match, params, REG_NOTBOL | REG_NOTEOL)) {
        printf("Levenshtein distance: %d\n", match.cost);
	if (match.cost != 1)
	{
		printf( "Edit distance mismatch in match\n");
		return 2;
	}
    } else {
        printf("Failed to match\n");
	return 1;
    }
    return 0;
}
