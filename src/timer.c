/*The MIT License

Copyright (c) 2010 Joshua Simmons and Jakob Ovrum

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
THE SOFTWARE.
*/


#include "timer.h"

#include <stdlib.h>
#include <sys/time.h>

/* private implementation dependent data, do not access directly from the outside */
struct TimerPriv
{
	struct timeval begin;
	struct timeval end;
};

Timer *sgl_timer_new()
{
	Timer *timer = malloc(sizeof(Timer));
	timer->priv = malloc(sizeof(struct TimerPriv));
	sgl_timer_reset(timer);
	return timer;
}

void sgl_timer_free(Timer *timer)
{
	free(timer->priv);
	free(timer);
}

void sgl_timer_reset(Timer *timer)
{
	gettimeofday(&(timer->priv->begin), NULL);
}

unsigned long sgl_timer_elapsed_milliseconds(Timer *timer)
{
	gettimeofday(&(timer->priv->end), NULL);
	return (timer->priv->end.tv_sec - timer->priv->begin.tv_sec) * 1000 + (timer->priv->end.tv_usec - timer->priv->begin.tv_usec) / 1000;
}

unsigned long sgl_timer_elapsed_microseconds(Timer *timer)
{
	gettimeofday(&(timer->priv->end), NULL);
	return (timer->priv->end.tv_sec - timer->priv->begin.tv_sec) * 1000000 + (timer->priv->end.tv_usec - timer->priv->begin.tv_usec);
}
