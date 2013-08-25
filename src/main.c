#include <nanomsg/nn.h>
#include <nanomsg/pubsub.h>



int main(int argc, char **argv) {
    int sock = nn_socket(AF_SP, NN_SUB);
}
