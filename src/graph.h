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
  double value1;
  double value2;
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
  double value;
  sds memory_key;
} GraphEdge;

typedef struct {
  List *nodes; //TODO: Use redis lists like the GraphNode
  List *edges; //TODO: Use redis lists like the GraphNode
  robj *nodes_hash;
  int directed;
} Graph;

robj *createGraphObject();
Graph* GraphCreate();
int GraphDirected(Graph* graph);
int GraphNodesSize(Graph* graph);
int GraphEdgesSize(Graph* graph);

List* ListCreate();
ListNode* ListNodeCreate(void* value);
void ListDeleteNode(List *, void *);
void ListAddNode(List *, ListNode *);

GraphNode* GraphNodeCreate(sds key);
void GraphAddNode(Graph *graph, GraphNode *node);
void GraphDeleteNode(Graph *graph, GraphNode *node);
GraphNode* GraphGetNode(Graph *graph, sds key);
int GraphNodeExists(Graph *graph, sds key);
GraphNode* GraphGetOrAddNode(Graph *graph, sds key);
int GraphNodeOutdegree(Graph *graph, GraphNode *node);

GraphEdge* GraphGetEdge(Graph *graph, GraphNode *node1, GraphNode *node2);
GraphEdge *GraphGetEdgeByKey(Graph *graph, sds key);
GraphEdge* GraphEdgeCreate(GraphNode *node1, GraphNode *node2, float value);
void GraphAddEdge(Graph *graph, GraphEdge *graph_edge);
void GraphDeleteEdge(Graph *graph, GraphEdge *graphEdge);

void freeGraphObject(robj *graph_object);

robj *neighboursToSet(GraphNode *node, Graph *graph_object);
