#include <stdio.h>
#include "network.h"
#include "image.h"
#include "data.h"
#include "utils.h"

#include "connected_layer.h"
#include "convolutional_layer.h"
//#include "old_conv.h"
#include "maxpool_layer.h"
#include "softmax_layer.h"

network make_network(int n)
{
    network net;
    net.n = n;
    net.layers = calloc(net.n, sizeof(void *));
    net.types = calloc(net.n, sizeof(LAYER_TYPE));
    net.outputs = 0;
    net.output = 0;
    return net;
}

void forward_network(network net, double *input)
{
    int i;
    for(i = 0; i < net.n; ++i){
        if(net.types[i] == CONVOLUTIONAL){
            convolutional_layer layer = *(convolutional_layer *)net.layers[i];
            forward_convolutional_layer(layer, input);
            input = layer.output;
        }
        else if(net.types[i] == CONNECTED){
            connected_layer layer = *(connected_layer *)net.layers[i];
            forward_connected_layer(layer, input);
            input = layer.output;
        }
        else if(net.types[i] == SOFTMAX){
            softmax_layer layer = *(softmax_layer *)net.layers[i];
            forward_softmax_layer(layer, input);
            input = layer.output;
        }
        else if(net.types[i] == MAXPOOL){
            maxpool_layer layer = *(maxpool_layer *)net.layers[i];
            forward_maxpool_layer(layer, input);
            input = layer.output;
        }
    }
}

void update_network(network net, double step, double momentum, double decay)
{
    int i;
    for(i = 0; i < net.n; ++i){
        if(net.types[i] == CONVOLUTIONAL){
            convolutional_layer layer = *(convolutional_layer *)net.layers[i];
            update_convolutional_layer(layer, step, momentum, decay);
        }
        else if(net.types[i] == MAXPOOL){
            //maxpool_layer layer = *(maxpool_layer *)net.layers[i];
        }
        else if(net.types[i] == SOFTMAX){
            //maxpool_layer layer = *(maxpool_layer *)net.layers[i];
        }
        else if(net.types[i] == CONNECTED){
            connected_layer layer = *(connected_layer *)net.layers[i];
            update_connected_layer(layer, step, momentum, 0);
        }
    }
}

double *get_network_output_layer(network net, int i)
{
    if(net.types[i] == CONVOLUTIONAL){
        convolutional_layer layer = *(convolutional_layer *)net.layers[i];
        return layer.output;
    } else if(net.types[i] == MAXPOOL){
        maxpool_layer layer = *(maxpool_layer *)net.layers[i];
        return layer.output;
    } else if(net.types[i] == SOFTMAX){
        softmax_layer layer = *(softmax_layer *)net.layers[i];
        return layer.output;
    } else if(net.types[i] == CONNECTED){
        connected_layer layer = *(connected_layer *)net.layers[i];
        return layer.output;
    }
    return 0;
}
double *get_network_output(network net)
{
    return get_network_output_layer(net, net.n-1);
}

double *get_network_delta_layer(network net, int i)
{
    if(net.types[i] == CONVOLUTIONAL){
        convolutional_layer layer = *(convolutional_layer *)net.layers[i];
        return layer.delta;
    } else if(net.types[i] == MAXPOOL){
        maxpool_layer layer = *(maxpool_layer *)net.layers[i];
        return layer.delta;
    } else if(net.types[i] == SOFTMAX){
        softmax_layer layer = *(softmax_layer *)net.layers[i];
        return layer.delta;
    } else if(net.types[i] == CONNECTED){
        connected_layer layer = *(connected_layer *)net.layers[i];
        return layer.delta;
    }
    return 0;
}

double *get_network_delta(network net)
{
    return get_network_delta_layer(net, net.n-1);
}

double calculate_error_network(network net, double *truth)
{
    double sum = 0;
    double *delta = get_network_delta(net);
    double *out = get_network_output(net);
    int i, k = get_network_output_size(net);
    for(i = 0; i < k; ++i){
        delta[i] = truth[i] - out[i];
        sum += delta[i]*delta[i];
    }
    return sum;
}

int get_predicted_class_network(network net)
{
    double *out = get_network_output(net);
    int k = get_network_output_size(net);
    return max_index(out, k);
}

double backward_network(network net, double *input, double *truth)
{
    double error = calculate_error_network(net, truth);
    int i;
    double *prev_input;
    double *prev_delta;
    for(i = net.n-1; i >= 0; --i){
        if(i == 0){
            prev_input = input;
            prev_delta = 0;
        }else{
            prev_input = get_network_output_layer(net, i-1);
            prev_delta = get_network_delta_layer(net, i-1);
        }
        if(net.types[i] == CONVOLUTIONAL){
            convolutional_layer layer = *(convolutional_layer *)net.layers[i];
            learn_convolutional_layer(layer);
            //learn_convolutional_layer(layer);
            //if(i != 0) backward_convolutional_layer(layer, prev_input, prev_delta);
        }
        else if(net.types[i] == MAXPOOL){
            maxpool_layer layer = *(maxpool_layer *)net.layers[i];
            if(i != 0) backward_maxpool_layer(layer, prev_input, prev_delta);
        }
        else if(net.types[i] == SOFTMAX){
            softmax_layer layer = *(softmax_layer *)net.layers[i];
            if(i != 0) backward_softmax_layer(layer, prev_input, prev_delta);
        }
        else if(net.types[i] == CONNECTED){
            connected_layer layer = *(connected_layer *)net.layers[i];
            learn_connected_layer(layer, prev_input);
            if(i != 0) backward_connected_layer(layer, prev_input, prev_delta);
        }
    }
    return error;
}

double train_network_datum(network net, double *x, double *y, double step, double momentum, double decay)
{
        forward_network(net, x);
        int class = get_predicted_class_network(net);
        double error = backward_network(net, x, y);
        update_network(net, step, momentum, decay);
        //return (y[class]?1:0);
        return error;
}

double train_network_sgd(network net, data d, int n, double step, double momentum,double decay)
{
    int i;
    double error = 0;
    for(i = 0; i < n; ++i){
        int index = rand()%d.X.rows;
        error += train_network_datum(net, d.X.vals[index], d.y.vals[index], step, momentum, decay);
        //if((i+1)%10 == 0){
        //    printf("%d: %f\n", (i+1), (double)correct/(i+1));
        //}
    }
    return error/n;
}
double train_network_batch(network net, data d, int n, double step, double momentum,double decay)
{
    int i;
    int correct = 0;
    for(i = 0; i < n; ++i){
        int index = rand()%d.X.rows;
        double *x = d.X.vals[index];
        double *y = d.y.vals[index];
        forward_network(net, x);
        int class = get_predicted_class_network(net);
        backward_network(net, x, y);
        correct += (y[class]?1:0);
    }
    update_network(net, step, momentum, decay);
    return (double)correct/n;

}


void train_network(network net, data d, double step, double momentum, double decay)
{
    int i;
    int correct = 0;
    for(i = 0; i < d.X.rows; ++i){
        correct += train_network_datum(net, d.X.vals[i], d.y.vals[i], step, momentum, decay);
        if(i%100 == 0){
            visualize_network(net);
            cvWaitKey(10);
        }
    }
    visualize_network(net);
    cvWaitKey(100);
    printf("Accuracy: %f\n", (double)correct/d.X.rows);
}

int get_network_output_size_layer(network net, int i)
{
    if(net.types[i] == CONVOLUTIONAL){
        convolutional_layer layer = *(convolutional_layer *)net.layers[i];
        image output = get_convolutional_image(layer);
        return output.h*output.w*output.c;
    }
    else if(net.types[i] == MAXPOOL){
        maxpool_layer layer = *(maxpool_layer *)net.layers[i];
        image output = get_maxpool_image(layer);
        return output.h*output.w*output.c;
    }
    else if(net.types[i] == CONNECTED){
        connected_layer layer = *(connected_layer *)net.layers[i];
        return layer.outputs;
    }
    else if(net.types[i] == SOFTMAX){
        softmax_layer layer = *(softmax_layer *)net.layers[i];
        return layer.inputs;
    }
    return 0;
}

int get_network_output_size(network net)
{
    int i = net.n-1;
    return get_network_output_size_layer(net, i);
}

image get_network_image_layer(network net, int i)
{
    if(net.types[i] == CONVOLUTIONAL){
        convolutional_layer layer = *(convolutional_layer *)net.layers[i];
        return get_convolutional_image(layer);
    }
    else if(net.types[i] == MAXPOOL){
        maxpool_layer layer = *(maxpool_layer *)net.layers[i];
        return get_maxpool_image(layer);
    }
    return make_empty_image(0,0,0);
}

image get_network_image(network net)
{
    int i;
    for(i = net.n-1; i >= 0; --i){
        image m = get_network_image_layer(net, i);
        if(m.h != 0) return m;
    }
    return make_empty_image(0,0,0);
}

void visualize_network(network net)
{
    int i;
    char buff[256];
    for(i = 0; i < net.n; ++i){
        sprintf(buff, "Layer %d", i);
        if(net.types[i] == CONVOLUTIONAL){
            convolutional_layer layer = *(convolutional_layer *)net.layers[i];
            visualize_convolutional_layer(layer, buff);
        }
    } 
}

double *network_predict(network net, double *input)
{
    forward_network(net, input);
    double *out = get_network_output(net);
    return out;
}

matrix network_predict_data(network net, data test)
{
    int i,j;
    int k = get_network_output_size(net);
    matrix pred = make_matrix(test.X.rows, k);
    for(i = 0; i < test.X.rows; ++i){
        double *out = network_predict(net, test.X.vals[i]);
        for(j = 0; j < k; ++j){
            pred.vals[i][j] = out[j];
        }
    }
    return pred;   
}

void print_network(network net)
{
    int i,j;
    for(i = 0; i < net.n; ++i){
        double *output = 0;
        int n = 0;
        if(net.types[i] == CONVOLUTIONAL){
            convolutional_layer layer = *(convolutional_layer *)net.layers[i];
            output = layer.output;
            image m = get_convolutional_image(layer);
            n = m.h*m.w*m.c;
        }
        else if(net.types[i] == MAXPOOL){
            maxpool_layer layer = *(maxpool_layer *)net.layers[i];
            output = layer.output;
            image m = get_maxpool_image(layer);
            n = m.h*m.w*m.c;
        }
        else if(net.types[i] == CONNECTED){
            connected_layer layer = *(connected_layer *)net.layers[i];
            output = layer.output;
            n = layer.outputs;
        }
        else if(net.types[i] == SOFTMAX){
            softmax_layer layer = *(softmax_layer *)net.layers[i];
            output = layer.output;
            n = layer.inputs;
        }
        double mean = mean_array(output, n);
        double vari = variance_array(output, n);
        fprintf(stderr, "Layer %d - Mean: %f, Variance: %f\n",i,mean, vari);
        if(n > 100) n = 100;
        for(j = 0; j < n; ++j) fprintf(stderr, "%f, ", output[j]);
        if(n == 100)fprintf(stderr,".....\n");
        fprintf(stderr, "\n");
    }
}

double network_accuracy(network net, data d)
{
    matrix guess = network_predict_data(net, d);
    double acc = matrix_accuracy(d.y, guess);
    free_matrix(guess);
    return acc;
}

