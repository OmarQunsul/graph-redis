typedef struct ListNode {
  void* value;
  struct ListNode *next;

} ListNode;

typedef struct {
  ListNode *root;
  ListNode *tail;
  int size;
} List;

typedef struct {
  float value;
  sds key;
  robj *edges;
  dict *edges_hash;
  robj *incoming; // Only for directed graphs
  sds memory_key;
  int visited; // TEMP
  struct GraphNode *parent;
} GraphNode;

typedef struct {
  GraphNode *node1;
  GraphNode *node2;
  float value;
  sds memory_key;
} GraphEdge;

typedef struct {
  List *nodes; //TODO: Use redis lists like the GraphNode
  List *edges; //TODO: Use redis lists like the GraphNode
  robj *nodes_hash;
  char directed;
} Graph;

robj *createGraphObject();
GraphNode* GraphNodeCreate(sds key, float value);
void GraphAddNode(Graph *graph, GraphNode *node);
GraphNode* GraphGetNode(Graph *graph, sds key);
GraphEdge* GraphEdgeCreate(GraphNode *node1, GraphNode *node2, float value);
void GraphAddEdge(Graph *graph, GraphEdge *graph_edge);

void ListDeleteNode(List *, void *);
void ListAddNode(List *, ListNode *);

void freeGraphObject(robj *graph_object);
