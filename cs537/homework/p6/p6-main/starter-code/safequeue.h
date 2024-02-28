// Priority queue structure
struct PQ_node {
    int client_fd;
    int priority;
    char *path;
    int delay;
};

// Function declarations
void create_queue(int max_queue_size);
int add_work(struct PQ_node * new_item);
struct PQ_node *get_work();
struct PQ_node *get_work_nonblocking();
void destroy_queue();
