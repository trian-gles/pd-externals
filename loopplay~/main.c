#include "m_pd.h"

static t_class *loopplay_tilde_class;

typedef struct _loopplay_tilde
{
    t_object x_obj;
    t_outlet *x_bangout;
    int x_phase;
    int x_nsampsintab;
    int x_limit;
    t_word *x_vec; // the array
    t_symbol *x_arrayname; // the name of the array
    t_clock *x_clock;
} t_loopplay_tilde;

void loopplay_tilde_tick(t_loopplay_tilde *x);

void *loopplay_tilde_new(t_symbol *s)
{
    t_loopplay_tilde *x = (t_loopplay_tilde *)pd_new(loopplay_tilde_class);
    x->x_clock = clock_new(x, (t_method)loopplay_tilde_tick);
    x->x_phase = 0x7fffffff; // current phase, sits at the maximum to not function
    x->x_limit = 0; // phase to stop at
    x->x_arrayname = s;
    outlet_new(&x->x_obj, &s_signal); // main output outlet
    x->x_bangout = outlet_new(&x->x_obj, &s_bang); //
    return (x);
}

t_int *loopplay_tilde_perform(t_int *w)
{
    t_loopplay_tilde *x = (t_loopplay_tilde *)(w[1]);
    t_sample *out = (t_sample *)(w[2]);
    t_word *wp;
    int n = (int)(w[3]), phase = x->x_phase,// n is the number of samples required in this method
        endphase = (x->x_nsampsintab < x->x_limit ? // the end phase is either the length of the table OR the x_limit arg
            x->x_nsampsintab : x->x_limit), nxfer, n3;
    if (!x->x_vec) // if there is no array, or we are past the endphase, go to zero
        goto zero;

    if (phase >= endphase)
    {
        phase = 0;
    }

    nxfer = endphase - phase; // nxfer stores the number of samples left before the end
    wp = x->x_vec + phase; // wp points to the current sample
    if (nxfer > n)
        nxfer = n; // nxfer now stores the amount of samples we WILL provide
    n3 = n - nxfer; // n3 stores the empty samples we will provide after nxfer runs out
    x->x_phase = (nxfer + phase) % endphase; // increment the phase
    while (nxfer--)
        *out++ = (wp++)->w_float;
    if (n3 > 0)
    {
        wp = x->x_vec;
        while (n3--)
            *out++ = (wp++)->w_float;
    }

    return (w+4);
zero:
    while (n--) *out++ = 0;
    return (w+4);
}

void loopplay_tilde_set(t_loopplay_tilde *x, t_symbol *s)
{
    t_garray *a;

    x->x_arrayname = s;
    if (!(a = (t_garray *)pd_findbyclass(x->x_arrayname, garray_class)))
    {
        if (*s->s_name) pd_error(x, "loopplay~: %s: no such array",
            x->x_arrayname->s_name);
        x->x_vec = 0;
    }
    else if (!garray_getfloatwords(a, &x->x_nsampsintab, &x->x_vec))
    {
        pd_error(x, "%s: bad template for loopplay~", x->x_arrayname->s_name);
        x->x_vec = 0;
    }
    else garray_usedindsp(a);
}

void loopplay_tilde_dsp(t_loopplay_tilde *x, t_signal **sp)
{
    loopplay_tilde_set(x, x->x_arrayname);
    dsp_add(loopplay_tilde_perform, 3, x, sp[0]->s_vec, (t_int)sp[0]->s_n);  //pass it this object, the output signal, and the number of samples requested
}

void loopplay_tilde_list(t_loopplay_tilde *x, t_symbol *s,
    int argc, t_atom *argv)
{
    long start = atom_getfloatarg(0, argc, argv);
    long length = atom_getfloatarg(1, argc, argv);
    if (start < 0) start = 0;
    if (length <= 0)
        x->x_limit = 0x7fffffff;
    else
        x->x_limit = (int)(start + length);
    x->x_phase = (int)start;
}

void loopplay_tilde_stop(t_loopplay_tilde *x)
{
    x->x_phase = 0x7fffffff;
}

void loopplay_tilde_tick(t_loopplay_tilde *x)
{
    outlet_bang(x->x_bangout);
}

void loopplay_tilde_free(t_loopplay_tilde *x)
{
    clock_free(x->x_clock);
}

void loopplay_tilde_setup(void)
{
    loopplay_tilde_class = class_new(gensym("loopplay~"),
        (t_newmethod)loopplay_tilde_new, (t_method)loopplay_tilde_free,
        sizeof(t_loopplay_tilde), 0, A_DEFSYM, 0);
    class_addmethod(loopplay_tilde_class, (t_method)loopplay_tilde_dsp,// dsp method
        gensym("dsp"), A_CANT, 0);
    class_addmethod(loopplay_tilde_class, (t_method)loopplay_tilde_stop,// i'm assuming this comes from a 0 being sent?
        gensym("stop"), 0);
    class_addmethod(loopplay_tilde_class, (t_method)loopplay_tilde_set,// what does DEFSYM do?
        gensym("set"), A_DEFSYM, 0);
    class_addlist(loopplay_tilde_class, loopplay_tilde_list);
}
