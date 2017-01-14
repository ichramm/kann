/*
  The MIT License

  Copyright (c) 2016  Broad Institute

  Permission is hereby granted, free of charge, to any person obtaining
  a copy of this software and associated documentation files (the
  "Software"), to deal in the Software without restriction, including
  without limitation the rights to use, copy, modify, merge, publish,
  distribute, sublicense, and/or sell copies of the Software, and to
  permit persons to whom the Software is furnished to do so, subject to
  the following conditions:

  The above copyright notice and this permission notice shall be
  included in all copies or substantial portions of the Software.

  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
  EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
  MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
  NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS
  BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN
  ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
  CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
  SOFTWARE.
*/

#ifndef KANN_H
#define KANN_H

#define KANN_VERSION "r368"

// #define NO_ATOMIC_BUILTIN // use this for VC++

#define KANN_F_IN       0x1   // input
#define KANN_F_OUT      0x2   // output
#define KANN_F_TRUTH    0x4   // truth output
#define KANN_F_COST     0x8   // final cost

#define KANN_C_CEB      1   // binary cross-entropy cost, used with sigmoid
#define KANN_C_CEM      2   // multi-class cross-entropy cost, used with softmax
#define KANN_C_CEB_NEG  3   // binary cross-enytopy-like cost, used with tanh

#define KANN_L_TEMP_INV (-1)

#include "kautodiff.h"

typedef struct {
	int n;            // number of nodes in the computational graph
	kad_node_t **v;   // list of nodes
	float *x, *g, *c; // collated variable values, gradients and constant values
} kann_t;

extern int kann_verbose;

#define kann_is_rnn(a) kad_unrollable((a)->n, (a)->v)
#define kann_size_var(a) kad_size_var((a)->n, (a)->v)
#define kann_size_const(a) kad_size_const((a)->n, (a)->v)
#define kann_dim_in(a) kann_feed_dim((a), KANN_F_IN, 0)
#define kann_dim_out(a) kann_feed_dim((a), KANN_F_TRUTH, 0)
#define kann_srand(seed) kad_srand(0, (seed))
#define kann_drand() kad_drand(0)

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Generate a network from a computational graph
 *
 * A network must have at least one scalar cost node (i.e. whose n_d==0). It
 * may optionally contain other cost nodes (i.e. for GAN) or output nodes.
 *
 * @param cost    cost node (must be a scalar, i.e. cost->n_d==0)
 * @param n_rest  number of other nodes without predecessors
 * @param ...     other nodes (of type kad_node_t*) without predecessors
 *
 * @return network on success, or NULL otherwise
 */
kann_t *kann_new(kad_node_t *cost, int n_rest, ...);

/**
 * Unroll an RNN
 *
 * @param a       network
 * @param len     number of unrolls
 *
 * @return an unrolled network, or NULL if the network is not an RNN
 */
kann_t *kann_unroll(kann_t *a, int len);

void kann_delete(kann_t *a);          // delete a network generated by kann_new() or kann_layer_final()
void kann_delete_unrolled(kann_t *a); // delete a network generated by kann_unroll()

/**
 * Set mini-batch size
 *
 * @param a         network
 * @param B         mini-batch size
 */
void kann_set_batch_size(kann_t *a, int B);

/**
 * Bind float arrays to feed nodes
 *
 * @param a         network
 * @param ext_flag  required external flags
 * @param ext_label required external label
 * @param x         pointers (size equal to the number of matching feed nodes)
 *
 * @return number of matching feed nodes
 */
int kann_feed_bind(kann_t *a, uint32_t ext_flag, int32_t ext_label, float **x);

/**
 * Compute the cost and optionall gradients
 *
 * @param a          network
 * @param cost_label required external label
 * @param cal_grad   whether to compute gradients
 *
 * @return cost
 */
float kann_cost(kann_t *a, int cost_label, int cal_grad);

int kann_eval(kann_t *a, uint32_t ext_flag, int ext_label);
int kann_class_error(const kann_t *ann);

/**
 * Find a node
 *
 * @param a         network
 * @param ext_flag  required external flags; set to 0 to match all flags
 * @param ext_label required external label
 *
 * @return >=0 if found; -1 if not found; -2 if found multiple
 */
int kann_find(const kann_t *a, uint32_t ext_flag, int32_t ext_label);

/**
 * Get the size of a feed node, assuming mini-batch size 1
 *
 * @param a         network
 * @param ext_flag  required external flags
 * @param ext_label required external label
 *
 * @return size>=0; -1 if not found; -2 if found multiple
 */
int kann_feed_dim(const kann_t *a, uint32_t ext_flag, int32_t ext_label);

/**
 * Get an RNN ready for continuous feeding
 *
 * @param a         network
 */
void kann_rnn_start(kann_t *a);

void kann_rnn_end(kann_t *a);

/**
 * Switch between training and prediction networks (effective only when there are switch nodes)
 *
 * @param a         network
 * @param is_train  0 for prediction network and non-zero for training net
 */
void kann_switch(kann_t *a, int is_train);

/**
 * RMSprop update
 *
 * @param n      number of variables
 * @param h0     learning rate
 * @param h      per-variable learning rate; NULL if not applicable
 * @param decay  RMSprop decay; use 0.9 if unsure
 * @param g      gradient, of size n
 * @param t      variables to change
 * @param r      memory, of size n
 */
void kann_RMSprop(int n, float h0, const float *h, float decay, const float *g, float *t, float *r);

float kann_grad_clip(float thres, int n, float *g);

// common layers
kad_node_t *kann_layer_input(int n1);
kad_node_t *kann_layer_linear(kad_node_t *in, int n1);
kad_node_t *kann_layer_dropout(kad_node_t *t, float r);
kad_node_t *kann_layer_rnn(kad_node_t *in, int n1, int var_h0);
kad_node_t *kann_layer_lstm(kad_node_t *in, int n1, int var_h0);
kad_node_t *kann_layer_gru(kad_node_t *in, int n1, int var_h0);
kad_node_t *kann_layer_conv2d(kad_node_t *in, int n_flt, int k_rows, int k_cols, int stride, int pad);
kad_node_t *kann_layer_max2d(kad_node_t *in, int k_rows, int k_cols, int stride, int pad);
kad_node_t *kann_layer_cost(kad_node_t *t, int n_out, int cost_type);

kad_node_t *kann_const_scalar(float x);
kad_node_t *kann_new_weight(int n_row, int n_col);
kad_node_t *kann_new_bias(int n);
kad_node_t *kann_new_weight_conv2d(int n_out_channel, int n_in_channel, int k_row, int k_col);
kad_node_t *kann_new_weight_conv1d(int n_out, int n_in, int kernel_len);

void kann_normal_array(float sigma, int n, float *x);

// operations on network with a single input node and a single output node
int kann_train_fnn1(kann_t *ann, float lr, int mini_size, int max_epoch, int max_drop_streak, float frac_val, int n, float **_x, float **_y);
const float *kann_apply1(kann_t *a, float *x);

// model I/O
void kann_save_fp(FILE *fp, kann_t *ann);
void kann_save(const char *fn, kann_t *ann);
kann_t *kann_load_fp(FILE *fp);
kann_t *kann_load(const char *fn);

#ifdef __cplusplus
}
#endif

#endif
