/*---------------------------------------------------------------
 * Copyright (c) 2017
 * Broadcom Corporation
 * All Rights Reserved.
 *---------------------------------------------------------------
 * Permission is hereby granted, free of charge, to any person
 * obtaining a copy of this software and associated
 * documentation files (the "Software"), to deal in the Software
 * without restriction, including without limitation the
 * rights to use, copy, modify, merge, publish, distribute,
 * sublicense, and/or sell copies of the Software, and to permit
 * persons to whom the Software is furnished to do
 * so, subject to the following conditions:
 *
 *
 * Redistributions of source code must retain the above
 * copyright notice, this list of conditions and
 * the following disclaimers.
 *
 *
 * Redistributions in binary form must reproduce the above
 * copyright notice, this list of conditions and the following
 * disclaimers in the documentation and/or other materials
 * provided with the distribution.
 *
 *
 * Neither the name of Broadcom Coporation,
 * nor the names of its contributors may be used to endorse
 * or promote products derived from this Software without
 * specific prior written permission.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES
 * OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NONINFRINGEMENT. IN NO EVENT SHALL THE CONTIBUTORS OR COPYRIGHT
 * HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
 * WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
 * ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 * ________________________________________________________________
 *
 * histograms.c
 * Suppport for histograms
 *
 * by Robert J. McMahon (rjmcmahon@rjmcmahon.com, bob.mcmahon@broadcom.com)
 * -------------------------------------------------------------------
 */
#include "headers.h"
#include "histogram.h"

histogram_t *histogram_init(unsigned int bincount, unsigned int binwidth, float offset, float units, char *name) {
    histogram_t *this = malloc(sizeof(histogram_t));
    this->mybins = malloc(sizeof(unsigned int) * bincount);
    this->myname = malloc(sizeof(strlen(name)));
    strcpy(this->myname, name);
    this->bincount = bincount;
    this->binwidth = binwidth;
    this->populationcnt = 0;
    this->offset=offset;
    this->units=units;
    this->cntloweroutofbounds=0;
    this->cntupperoutofbounds=0;
    this->prev = NULL;
    return this;
}

void histogram_delete(histogram_t *h) {
    if (h->mybins)
	free(h->mybins);
    if (h->myname)
	free(h->myname);
    if (h->prev)
	histogram_delete(h->prev);
    free(h);
}

int histogram_insert(histogram_t *h, float value) {
    int bin;
    // calculate the bin
    bin = (int) (h->units *(value - h->offset) / h->binwidth);
    h->populationcnt++;
    if (bin < 0) {
	h->cntloweroutofbounds++;
	return(-1);
    } else if (bin > (int) h->bincount) {
	h->cntupperoutofbounds++;
	return(-2);
    }
    else {
	h->mybins[bin]++;
	return(h->mybins[bin]);
    }
}

void histogram_clear(histogram_t *h) {
    int ix;
    for (ix = 0; ix < h->bincount; ix++) {
	h->mybins[ix]=0;
    }
}

void histogram_add(histogram_t *to, histogram_t *from) {
    int ix;
    for (ix=0; ix < to->bincount; ix ++) {
	to->mybins[ix] += from->mybins[ix];
    }
}

void histogram_print(histogram_t *h) {
    char *buf = malloc((20*h->bincount)+strlen(h->myname));
    int n = 0, ix;
    sprintf(buf, "%s(%d,%d) ", h->myname, h->bincount, h->binwidth);
    n = strlen(buf);
    for (ix = 0; ix < h->bincount; ix++) {
	if (h->mybins[ix] > 0) {
	    n += sprintf(buf + n,"%d:%d,", ix, h->mybins[ix]);
	}
    }
    buf[strlen(buf)-1]=0;
    fprintf(stdout, "%s\n", buf);
    free(buf);
}

void histogram_print_interval(histogram_t *h) {
    if (!h->prev) {
	h->prev = histogram_init(h->bincount, h->binwidth, h->offset, h->units, h->myname);
    }
    char *buf = malloc((20*h->bincount)+strlen(h->myname));
    int n = 0, ix, delta, lowerci, upperci;
    int running=0;
    int intervalpopulation, oob_u, oob_l;
    if (h->units > 1e3) {
	sprintf(buf, "%s(bw=%dus) ", h->myname,h->binwidth);
    } else {
	sprintf(buf, "%s(bw=%dms) ", h->myname,h->binwidth);
    }
    n = strlen(buf);
    lowerci=0;
    upperci=0;
    intervalpopulation = h->populationcnt - h->prev->populationcnt;
    h->prev->populationcnt = h->populationcnt;
    oob_l = h->cntloweroutofbounds - h->prev->cntloweroutofbounds;
    h->prev->cntloweroutofbounds = h->cntloweroutofbounds;
    oob_u = h->cntupperoutofbounds - h->prev->cntupperoutofbounds;
    h->prev->cntupperoutofbounds = h->cntupperoutofbounds;

    for (ix = 0; ix < h->bincount; ix++) {
	delta = h->mybins[ix] - h->prev->mybins[ix];
	if (delta > 0) {
	    running+=delta;
	    if (!lowerci && ((float)running/intervalpopulation > 0.05)) {
		lowerci = ix;
	    }
	    if (!upperci && ((float)running/intervalpopulation > 0.95)) {
		upperci = ix;
	    }
	    n += sprintf(buf + n,"%d:%d,", ix, delta);
	    h->prev->mybins[ix] = h->mybins[ix];
	}
    }
    buf[strlen(buf)-1] = 0;
    if (!upperci)
       upperci=h->bincount;
    fprintf(stdout, "%s (5/95%%=%d/%d,obl/obu=%d/%d)\n", buf, lowerci, upperci, oob_l, oob_u);
    free(buf);
}
