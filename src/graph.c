#include "server.h"
#include "graph.h"
#include <math.h> /* isnan(), isinf() */

void freeGraphObject(robj *graph) {
  Graph *graphObject = (Graph *)(graph->ptr);
  List *graphEdges = graphObject->edges;
  ListNode *current_node = graphEdges->root;

  // Deleting Edges
  while (current_node != NULL) {
    GraphEdge *graphEdge = (GraphNode *)(current_node->value);
    GraphDeleteEdge(graphObject, graphEdge);

    current_node = current_node->next;
  }

  // Deleting Nodes
  List *graphNodes = graphObject->nodes;
  current_node = graphNodes->root;


  while (current_node != NULL) {
    GraphNode *graphNode = (GraphNode *)(current_node->value);
    GraphDeleteNode(graphObject, graphNode);
    current_node = current_node->next;
  }
}

ListNode* ListNodeCreate(void* value) {
  ListNode* listNode = (ListNode *)zmalloc(sizeof(ListNode));
  listNode->value = value;
  listNode->next = NULL;
  return listNode;
}

List* ListCreate() {
  List* list = zmalloc(sizeof(List));
  list->root = NULL;
  list->tail = NULL;
  list->size = 0;
  return list;
}

GraphNode* GraphNodeCreate(sds key) {
  GraphNode* graphNode = zmalloc(sizeof(GraphNode));
  graphNode->key = sdsdup(key);
  graphNode->edges = createQuicklistObject();

  quicklistSetOptions(graphNode->edges->ptr, server.list_max_ziplist_size,
      server.list_compress_depth);

  graphNode->edges_hash = dictCreate(&dbDictType, NULL);
  graphNode->incoming = createQuicklistObject();
  graphNode->visited = 0;

  // unique Node key
  graphNode->memory_key = sdsfromlonglong((unsigned long)(graphNode));

  return graphNode;
}

GraphEdge* GraphEdgeCreate(GraphNode *node1, GraphNode *node2, float value) {
  GraphEdge* graphEdge = zmalloc(sizeof(GraphEdge));
  graphEdge->node1 = node1;
  graphEdge->node2 = node2;
  graphEdge->value = value;

  // unique Edge key
  graphEdge->memory_key = sdsfromlonglong((unsigned long)(graphEdge));

  // Just for testing to make sure it is working
  GraphEdge* u2 = (GraphEdge *)((unsigned long)(atol(graphEdge->memory_key)));
  serverAssert(u2 == graphEdge);

  return graphEdge;
}

void GraphAddNode(Graph *graph, GraphNode *node) {
  ListNode* listNode = ListNodeCreate((void *)(node));
  ListAddNode(graph->nodes, listNode);

  // edges hash
  serverAssert(dictAdd(graph->nodes_hash, node->key, node) == DICT_OK);
}

void GraphDeleteNode(Graph *graphObject, GraphNode *node) {
  GraphEdge *edge;
  int i;
  robj *list = node->edges;
  long count = listTypeLength(list);

  quicklistEntry entry;

  for (i = 0; i < count; i++) {
    quicklistIndex(list->ptr, i, &entry);
    sds value;
    value = sdsfromlonglong(entry.longval);
    edge = GraphGetEdgeByKey(graphObject, value);
    sdsfree(value);
    serverAssert(edge != NULL);
    GraphDeleteEdge(graphObject, edge);
  }

  if (GraphDirected(graphObject)) {
    list = node->incoming;
    count = listTypeLength(list);

    quicklistEntry entry;

    for (i = 0; i < count; i++) {
      quicklistIndex(list->ptr, i, &entry);
      sds value;
      value = sdsfromlonglong(entry.longval);
      edge = GraphGetEdgeByKey(graphObject, value);
      sdsfree(value);
      serverAssert(edge != NULL);
      GraphDeleteEdge(graphObject, edge); //TODO: think about it
    }
  }

  // Deleting the node
  ListDeleteNode(graphObject->nodes, node);
  dictUnlink(graphObject->nodes_hash, node->key);
}

void GraphAddEdge(Graph *graph, GraphEdge *graphEdge) {
  ListNode* listNode = ListNodeCreate((void *)(graphEdge));
  ListAddNode(graph->edges, listNode);

  // Node 1 edges
  // edges list
  listTypePush(graphEdge->node1->edges, createStringObject(graphEdge->memory_key, sdslen(graphEdge->memory_key)), LIST_TAIL);
  // edges hash
  dictAdd(graphEdge->node1->edges_hash, graphEdge->node2->key, graphEdge);

  // Node 2 edges
  if (GraphDirected(graph)) {
    listTypePush(graphEdge->node2->incoming, createStringObject(graphEdge->memory_key, sdslen(graphEdge->memory_key)), LIST_TAIL);
  }
  else { // Undirected
    listTypePush(graphEdge->node2->edges, createStringObject(graphEdge->memory_key, sdslen(graphEdge->memory_key)), LIST_TAIL);
    dictAdd(graphEdge->node2->edges_hash, graphEdge->node1->key, graphEdge); // == DICT_OK;
  }
}

void GraphDeleteEdge(Graph *graph, GraphEdge *graphEdge) {
  listTypeEntry entry;
  listTypeIterator *li;

  GraphNode *node1 = graphEdge->node1;
  GraphNode *node2 = graphEdge->node2;

  bool equal;
  robj * key;

  // Deleting from node1
  li = listTypeInitIterator(node1->edges, 0, LIST_TAIL);
  while (listTypeNext(li,&entry)) {
    key = createStringObject(graphEdge->memory_key, sdslen(graphEdge->memory_key));
    equal = listTypeEqual(&entry, key);
    decrRefCount(key);
    if (equal) {
      listTypeDelete(li, &entry);
      break;
    }
  }
  listTypeReleaseIterator(li);
  // Deleting from node1 hash
  dictUnlink(node1->edges_hash, graphEdge->node2->key);

  // Deleting from node2
  if (GraphDirected(graph)) {
    li = listTypeInitIterator(node2->incoming, 0, LIST_TAIL);
  }
  else {
    li = listTypeInitIterator(node2->edges, 0, LIST_TAIL);
  }

  while (listTypeNext(li,&entry)) {
    key = createStringObject(graphEdge->memory_key, sdslen(graphEdge->memory_key));
    equal = listTypeEqual(&entry, key);
    decrRefCount(key);
    if (equal) {
      if (GraphDirected(graph)) {
      }
      listTypeDelete(li, &entry);
      break;
    }
  }
  listTypeReleaseIterator(li);
  if (!GraphDirected(graph)) {
    // Deleting from node2 hash
    dictUnlink(node2->edges_hash, graphEdge->node1->key);
  }

  // Deleting from Graph
  ListDeleteNode(graph->edges, graphEdge);
}

Graph* GraphCreate() {
  Graph* graph = zmalloc(sizeof(Graph));
  graph->nodes = ListCreate();
  graph->edges = ListCreate();
  graph->nodes_hash = dictCreate(&dbDictType, NULL);
  graph->directed = 0;
  return graph;
}

int GraphDirected(Graph* graph) {
  return graph->directed != 0;
}

int GraphNodesSize(Graph* graph) {
  return graph->nodes->size;
}

int GraphEdgesSize(Graph* graph) {
  return graph->nodes->size;
}

GraphNode* GraphGetNode(Graph *graph, sds key) {
  dictEntry *entry = dictFind(graph->nodes_hash, key);

  if (entry == NULL) return NULL;
  GraphNode *node = (GraphNode *)(dictGetVal(entry));

  return node;
}

int GraphNodeOutdegree(Graph *graph, GraphNode *node) {
  return listTypeLength(node->edges);
}

int GraphNodeIndegree(Graph *graph, GraphNode *node) {
  return listTypeLength(node->incoming);
}

GraphNode* GraphGetOrAddNode(Graph *graph, sds key) {
  dictEntry *entry = dictFind(graph->nodes_hash, key);

  GraphNode *node;
  if (entry == NULL) {
    node = GraphNodeCreate(key);
    GraphAddNode(graph, node);
  } else {
    node = (GraphNode *)(dictGetVal(entry));
  }
  return node;
}

int GraphNodeExists(Graph *graph, sds key) {
  GraphNode *node = GraphGetNode(graph, key);
  return (node != NULL);
}

GraphEdge* GraphGetEdge(Graph *graph, GraphNode *node1, GraphNode *node2) {
  dictEntry *entry = dictFind(node1->edges_hash, node2->key);
  if (entry == NULL) return NULL;
  GraphEdge *edge = (GraphEdge *)(dictGetVal(entry));
  return edge;
}

GraphEdge *GraphGetEdgeByKey(Graph *graph, sds key) {
  GraphEdge *edge = (GraphEdge *)((unsigned long)(atol(key)));
  return edge;
}

void ListAddNode(List *list, ListNode *node) {
  if (list->root == NULL) {
    list->root = node;
    list->tail = node;
  } else {
    list->tail->next = node;
    list->tail = node;
  }
  list->size++;
}

// TODO: Fix for the tail
void ListDeleteNode(List *list, void *value) {
  ListNode* previous = NULL;
  ListNode* current = list->root;

  while (current != NULL) {
    if ((void *)(current->value) == value) {
      if (previous) {
        previous->next = current->next;
      } else {
        list->root = current->next;
      }
      zfree(current);
      list->size--;

      return;
    }

    previous = current;
    current = current->next;
  }

  return;
}

robj *createGraphObject() {
  Graph *ptr = GraphCreate();
  robj *obj = createObject(OBJ_GRAPH, ptr);
  obj->encoding = OBJ_GRAPH;
  return obj;
}

robj *neighboursToSet(GraphNode *node, Graph *graphObject) {
  // Creating neighbours set of the first node

  GraphEdge *edge;
  robj *set;
  sds edge_key;

  set = setTypeCreate(node->key);

  long count;

  count = listTypeLength(node->edges);
  robj *list = node->edges;
  int i;

  quicklistEntry entry;

  for (i = 0; i < count; i++) {
    quicklistIndex(list->ptr, i, &entry);
    edge_key = sdsfromlonglong(entry.longval);
    edge = GraphGetEdgeByKey(graphObject, edge_key);
    sdsfree(edge_key);
    sds neighbour_key;
    if (sdscmp(edge->node1->key, node->key) == 0) {
      neighbour_key = sdsdup(edge->node2->key);
    } else {
      neighbour_key = sdsdup(edge->node1->key);
    }
    setTypeAdd(set, neighbour_key);
  }

  return set;
}

