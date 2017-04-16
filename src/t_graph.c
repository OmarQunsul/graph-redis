#include "server.h"
#include "graph.h"
#include "t_graph.h"
#include <math.h> /* isnan(), isinf() */

#define RETURN_OK \
  robj* value10 = createStringObject("OK", strlen("OK")); \
  addReplyBulk(c, value10); \
  decrRefCount(value10);


#define CHECK_GRAPH_OR_CREATE \
  if (graph == NULL) { \
    graph = createGraphObject(); \
    dbAdd(c->db, key, graph); \
  } else if (graph->type != OBJ_GRAPH) { \
    addReply(c, shared.wrongtypeerr); \
    return C_ERR; \
  }

#define CHECK_GRAPH_EXISTS \
  if (!graph || graph->type != OBJ_GRAPH) { \
    addReply(c, shared.wrongtypeerr); \
    return C_ERR; \
  }

void gshortestpathCommand(client *c) {
  robj *graph;
  robj *key = c->argv[1];
  graph = lookupKeyRead(c->db, key);
  CHECK_GRAPH_EXISTS
  Graph *graphObject = (Graph *)(graph->ptr);

  GraphNode *node1 = GraphGetNode(graphObject, c->argv[2]->ptr);
  if (node1 == NULL) {
    sds err = sdsnew("Node not found");
    addReplyError(c, err);
    return C_ERR;
  }

  GraphNode *node2 = GraphGetNode(graphObject, c->argv[3]->ptr);
  if (node2 == NULL) {
    sds err = sdsnew("Node not found");
    addReplyError(c, err);
    return C_ERR;
  }

  ListNode *current_node;
  current_node = graphObject->nodes->root;
  while (current_node != NULL) {
    GraphNode *graphNode = (GraphNode *)(current_node->value);
    graphNode->parent = NULL;
    graphNode->visited = 0;
    current_node = current_node->next;
  }

  serverAssert(node1 != NULL);
  serverAssert(node2 != NULL);

  dijkstra(c, graphObject, node1, node2);
}

void dijkstra(client *c, Graph *graph, GraphNode *node1, GraphNode *node2) {
  robj *distances_obj = createZsetObject();
  zset *distances = distances_obj->ptr;

  // Initialization
  sds keyk = sdsdup(node1->key);
  zslInsert(distances->zsl, 0, keyk);
  dictAdd(distances->dict, keyk, NULL);

  // Main loop
  GraphNode *current_node = node1;
  double current_node_distance;

  current_node_distance = 0;
  int finished = 0;
  double final_distance;
  zskiplistNode *tmp_node = NULL;

  while (current_node != NULL) {
    // Reached Destination, and print the total distance
    if (sdscmp(current_node->key, node2->key) == 0) {
      final_distance = current_node_distance;
      finished = 1;
      break;
    }

    // Deleting the top of the distances
    zslDelete(distances->zsl, current_node_distance, current_node->key, &tmp_node);
    zslFreeNode(tmp_node);
    dictDelete(distances->dict, current_node->key);

    // Marking the node as visited
    current_node->visited = 1;

    int neighbours_count = listTypeLength(current_node->edges);
    int j;

    GraphEdge *edge;

    for (j = 0; j < neighbours_count; j++) {
      quicklistEntry entry;

      quicklistIndex(current_node->edges->ptr, j, &entry);

      sds key = sdsfromlonglong(entry.longval);
      edge = GraphGetEdgeByKey(graph, key);
      sdsfree(key);

      GraphNode *neighbour = NULL;

      if (edge->node1 == current_node) {
        neighbour = edge->node2;
      } else if (edge->node2 == current_node) {
        neighbour = edge->node1;
      }

      if (neighbour != NULL) {
        if (neighbour->visited) continue;

        double distance = edge->value + current_node_distance;
        double neighbour_distance;

        dictEntry *de;
        de = dictFind(distances->dict, neighbour->key);

        if (de != NULL) {
          neighbour_distance = *((double *)dictGetVal(de));
          if (distance < neighbour_distance) {
            // Deleting
            zslDelete(distances->zsl, neighbour_distance, neighbour->key, &tmp_node);
            dictDelete(distances->dict, neighbour->key);
            zslFreeNode(tmp_node);
            //if (tmp_node != NULL) zfree(tmp_node);
            // Inserting again
            sds key2 = sdsdup(neighbour->key);
            zslInsert(distances->zsl, distance, key2);
            // Update the parent
            neighbour->parent = current_node;
          }
        } else {
          sds key3 = sdsdup(neighbour->key);
          zslInsert(distances->zsl, distance, key3);
          double *double_loc = zmalloc(sizeof(double));
          *double_loc = distance;
          dictAdd(distances->dict, key3, double_loc);
          neighbour->parent = current_node;
        }
      }
    }
    // READING MINIMUM
    // FOOTER
    zskiplistNode *first_node = distances->zsl->header->level[0].forward;

    robj *key;

    if (!finished && first_node != NULL) {
      current_node = GraphGetNode(graph, first_node->ele);
      current_node_distance = first_node->score;
      //zslFreeNode(first_node);
    } else  {
      current_node = NULL;
    }

  }

  // Building reply

  GraphNode *cur_node = node2;
  int count = 1;

  while(cur_node != NULL) {
    cur_node = cur_node->parent;
    if (cur_node != NULL)
      count++;
    else {
      // NO PATH
      addReply(c, shared.czero);
      return C_OK;
    }
    if (sdscmp(cur_node->key, node1->key) == 0) {
      break;
    }
  }

  addReplyMultiBulkLen(c, count + 1);
  cur_node = node2;

  robj **replies = zmalloc(sizeof(robj *) * count);
  int k = count - 1;
  replies[k--] = createStringObject(node2->key, sdslen(node2->key));

  while(cur_node != NULL) {
    cur_node = cur_node->parent;
    if (cur_node != NULL) {
      replies[k--] = createStringObject(cur_node->key, sdslen(cur_node->key));
    }
    if (sdscmp(cur_node->key, node1->key) == 0) {
      break;
    }
  }
  serverAssert(k == -1);

  // Path nodes reversed
  for(k = 0; k < count; k++) {
    addReplyBulk(c, replies[k]);
    decrRefCount(replies[k]);
  }

  robj *distance_reply = createStringObjectFromLongDouble(final_distance, 0);
  addReplyBulk(c, distance_reply);
  decrRefCount(distance_reply);

  // Clear memory
  zfree(replies);
  decrRefCount(distances_obj);

  return C_OK;
}

void gmintreeCommand(client *c) {
  // Using Prim's Algorithm
  robj *graph;
  robj *key = c->argv[1];
  graph = lookupKeyRead(c->db, key);
  CHECK_GRAPH_EXISTS
  Graph *graphObject = (Graph *)(graph->ptr);

  robj *graph2_key = c->argv[2];

  robj *graph2 = createGraphObject();
  Graph *graph2_object = (Graph *)(graph2->ptr);
  dbAdd(c->db, graph2_key, graph2);

  // Adding first node
  GraphNode *node, *root;
  root = (GraphNode *)(graphObject->nodes->root->value);
  node = GraphNodeCreate(root->key, 0);
  GraphAddNode(graph2_object, node);

  // Creating a priority-queue for edges
  robj *queue = createZsetObject();
  zset *qzs = queue->ptr;

  // TODO: Make sure first node has edges, and that's not a problem, but it should be a connected graph !

  // Insert the first node edges to the queue
  robj *list = root->edges;
  long count = listTypeLength(list);
  quicklistEntry entry;

  int i;
  GraphEdge *edge;
  for(i = 0; i < count; i++) {
    quicklistIndex(list->ptr, i, &entry);
    sds value;
    value = sdsfromlonglong(entry.longval);
    edge = GraphGetEdgeByKey(graphObject, value);
    sdsfree(value);
    zslInsert(qzs->zsl, edge->value, edge->memory_key);
  }

  // While the minimum edge connects existing node to new node, or BETTER: until the
  // new graph nodes length == graph 1 nodes length
  while (1) {
    zskiplistNode *node;
    node = qzs->zsl->header->level[0].forward;
    if (node != NULL) {
      GraphEdge *edge = GraphGetEdgeByKey(graphObject, node->ele);
      zslDelete(qzs->zsl, node->score, edge->memory_key, &node); // MODIFIED
      zslFreeNode(node);
      GraphNode *node1, *node2;
      node1 = edge->node1;
      node2 = edge->node2;

      char a, b;
      a = GraphNodeExists(graph2_object, node1->key);
      b = GraphNodeExists(graph2_object, node2->key);

      if (a ^ b) {

        // HERE: Add the new node to graph2
        GraphNode *node_to_add = a ? GraphNodeCreate(node2->key, 0) : GraphNodeCreate(node1->key, 0);
        GraphAddNode(graph2_object, node_to_add);

        // Adding the edge to graph2
        GraphNode *temp = GraphGetNode(graph2_object, a ? node1->key: node2->key);
        GraphEdge *new_edge = GraphEdgeCreate(temp, node_to_add, edge->value);
        GraphAddEdge(graph2_object, new_edge);

        // Adding the node edges to the queue, if they don't exist before
        list = (a ? node2 : node1)->edges;
        count = listTypeLength(list);
        for(i = 0; i < count; i++) {
          GraphEdge *edge2;
          quicklistIndex(list->ptr, i, &entry);
          sds value = sdsfromlonglong(entry.longval);
          edge2 = GraphGetEdgeByKey(graphObject, value);
          //zfree(entry);

          GraphNode *g2_node1 = GraphGetNode(graph2_object, edge2->node1->key);
          GraphNode *g2_node2 = GraphGetNode(graph2_object, edge2->node2->key);
          // Check if the edge already exists in graph2
          //GraphNode *tmp_edge = GraphGetEdgeByKey(graph2_object, edge_key);
          if ((g2_node1 != NULL) && (g2_node2 != NULL)) {
            // Nothing
          } else {
            zslInsert(qzs->zsl, edge2->value, edge2->memory_key);
          }
        }

      }

    } else {
      break;
    }
  }

  RETURN_OK
    return;

}

void gsetdirectedCommand(client *c) {
  robj *graph;
  robj *key = c->argv[1];
  graph = lookupKeyWrite(c->db, key);
  CHECK_GRAPH_EXISTS
  Graph *graphObject = (Graph *)(graph->ptr);
  graphObject->directed = 1;

  RETURN_OK
}

void gnodeCommand(client *c) {
  robj *graph;
  robj *key = c->argv[1];
  graph = lookupKeyWrite(c->db, key);
  CHECK_GRAPH_OR_CREATE

  Graph *graphObject = (Graph *)(graph->ptr);

  int added = 0;

  int i;
  for (i = 2; i < (c->argc); i++) {
    sds key = sdsnew(c->argv[i]->ptr);
    GraphNode *graph_node = GraphGetNode(graphObject, key);
    if (graph_node == NULL) {
      graph_node = GraphNodeCreate(key, 0);
      GraphAddNode(graphObject, graph_node);
      added++;
    } else {
      sdsfree(key);
    }
  }

  addReplyLongLong(c, added);
  return C_OK;
}

void gincomingCommand(client *c) {
  robj *graph;
  robj *edge_key;
  robj *key = c->argv[1];
  graph = lookupKeyRead(c->db, key);
  CHECK_GRAPH_EXISTS
  GraphEdge *edge;
  Graph *graphObject = (Graph *)(graph->ptr);
  GraphNode *node = GraphGetNode(graphObject, c->argv[2]->ptr);

  // Neighbours count
  int i;
  robj *list = node->incoming;
  long count = listTypeLength(list);
  addReplyMultiBulkLen(c, count);

  quicklistEntry entry;

  for (i = 0; i < count; i++) {
    quicklistIndex(list->ptr, i, &entry);
    robj *value;
    value = sdsfromlonglong(entry.longval);
    edge = GraphGetEdgeByKey(graphObject, value);
    sdsfree(value);
    serverAssert(edge != NULL);
    if (sdscmp(edge->node1->key, node->key) == 0) {
      addReplyBulk(c, createStringObject(sdsdup(edge->node2->key), sdslen(edge->node2->key)));
    } else {
      addReplyBulk(c, createStringObject(sdsdup(edge->node1->key), sdslen(edge->node1->key)));
    }
  }

  return C_OK;
}

void gmaxneighbourCommand(client *c) {
  robj *graph;
  robj *edge_key;
  robj *key = c->argv[1];
  graph = lookupKeyRead(c->db, key);
  CHECK_GRAPH_EXISTS
  GraphEdge *edge;
  Graph *graphObject = (Graph *)(graph->ptr);
  GraphNode *node = GraphGetNode(graphObject, c->argv[2]->ptr);

  int i;
  robj *list = node->edges;
  long count = listTypeLength(list);

  quicklistEntry entry;

  if (count == 0) {
    addReply(c, shared.czero);
    return C_OK;
  } else {
    addReplyMultiBulkLen(c, 2);
    int max_value;
    sds max_key = sdsempty();

    for (i = 0; i < count; i++) {
      quicklistIndex(list->ptr, i, &entry);
      sds value;
      value = sdsfromlonglong(entry.longval);
      edge = GraphGetEdgeByKey(graphObject, value);
      sdsfree(value);
      serverAssert(edge != NULL);

      if (sdslen(max_key) == 0 || edge->value >= max_value) {
        sdsfree(max_key);
        max_value = edge->value;
        if (sdscmp(edge->node1->key, node->key) == 0) {
          max_key = sdsdup(edge->node2->key);
        } else {
          max_key = sdsdup(edge->node1->key);
        }
      }
    }

    addReplyBulk(c, createStringObject(sdsdup(max_key), sdslen(max_key)));
    addReplyLongLong(c, max_value);
    sdsfree(max_key);
    return C_OK;
  }
}

void gneighboursCommand(client *c) {
  robj *graph;
  robj *edge_key;
  robj *key = c->argv[1];
  graph = lookupKeyRead(c->db, key);
  CHECK_GRAPH_EXISTS
  GraphEdge *edge;
  Graph *graphObject = (Graph *)(graph->ptr);
  GraphNode *node = GraphGetNode(graphObject, c->argv[2]->ptr);

  // Neighbours count
  int i;
  robj *list = node->edges;
  long count = listTypeLength(list);
  addReplyMultiBulkLen(c, count);

  quicklistEntry entry;

  for (i = 0; i < count; i++) {
    quicklistIndex(list->ptr, i, &entry);
    sds value;
    value = sdsfromlonglong(entry.longval);
    edge = GraphGetEdgeByKey(graphObject, value);
    sdsfree(value);
    serverAssert(edge != NULL);
    if (sdscmp(edge->node1->key, node->key) == 0) {
      addReplyBulk(c, createStringObject(sdsdup(edge->node2->key), sdslen(edge->node2->key)));
    } else {
      addReplyBulk(c, createStringObject(sdsdup(edge->node1->key), sdslen(edge->node1->key)));
    }
  }

  return C_OK;
}

void gcommonCommand(client *c) {
  robj *graph;
  robj *key = c->argv[1];
  graph = lookupKeyRead(c->db, key);
  CHECK_GRAPH_EXISTS
  Graph *graphObject = (Graph *)(graph->ptr);
  GraphNode *node1 = GraphGetNode(graphObject, c->argv[2]->ptr);
  GraphNode *node2 = GraphGetNode(graphObject, c->argv[3]->ptr);

  if (node1 == NULL || node2 == NULL) {
    addReplyMultiBulkLen(c, 0);
    return C_OK;
  }

  robj *set1 = NULL;
  robj *set2 = NULL;

  set1 = neighboursToSet(node1, graphObject);
  set2 = neighboursToSet(node2, graphObject);

  if (set1 == NULL || set2 == NULL) {
    addReplyMultiBulkLen(c, 0);
    return C_OK;
  }

  //robj *result = createIntsetObject();
  robj *result = createSetObject();

  // Switch set1, and set2 if set1 length is bigger. To improve performance
  robj *temp;

  setTypeIterator *si = setTypeInitIterator(set1);
  int encoding;
  int intobj;
  sds *eleobj;

  while(eleobj = setTypeNextObject(si)) {
    char *l = eleobj; // FOR DEBUGGING
    if (setTypeIsMember(set2, eleobj)){
      setTypeAdd(result, eleobj);
    }
  }
  setTypeReleaseIterator(si);

  addReplyMultiBulkLen(c, setTypeSize(result));

  si = setTypeInitIterator(result);
  while(eleobj = setTypeNextObject(si)) {
    addReplyBulk(c, createStringObject(eleobj, sdslen(eleobj)));
  }
  setTypeReleaseIterator(si);

  freeSetObject(set1); // decReft
  freeSetObject(set2);
  freeSetObject(result);

  return C_OK;
}

void gnodeexistsCommand(client *c) {
  robj *graph;
  robj *key = c->argv[1];
  graph = lookupKeyRead(c->db, key);
  CHECK_GRAPH_EXISTS

  Graph *graphObject = (Graph *)(graph->ptr);
  GraphNode *graph_node = GraphGetNode(graphObject, c->argv[2]->ptr);
  if (graph_node != NULL) {
    addReply(c, shared.cone);
  } else {
    addReply(c, shared.czero);
  }
  return C_OK;
}

void gedgeexistsCommand(client *c) {
  robj *graph;
  GraphEdge *edge;
  robj *key = c->argv[1];
  graph = lookupKeyRead(c->db, key);
  CHECK_GRAPH_EXISTS

  Graph *graphObject = (Graph *)(graph->ptr);
  GraphNode *graph_node1 = GraphGetNode(graphObject, c->argv[2]->ptr);
  GraphNode *graph_node2 = GraphGetNode(graphObject, c->argv[3]->ptr);

  // Return zero if any of the nodes is/are null
  if ((graph_node1 == NULL) || (graph_node2 == NULL)) {
    addReply(c, shared.czero);
    return C_OK;
  }

  // Check whether the edge already exists
  edge = NULL;
  if (graph_node1 != NULL && graph_node2 != NULL)
    edge = GraphGetEdge(graphObject, graph_node1, graph_node2);

  if (edge != NULL) {
    addReply(c, shared.cone);
  } else {
    addReply(c, shared.czero);
  }
  return C_OK;
}

void gedgevalueCommand(client *c) {
  robj *graph;
  robj *key = c->argv[1];
  graph = lookupKeyRead(c->db, key);
  CHECK_GRAPH_EXISTS

  GraphEdge *edge;
  Graph *graphObject = (Graph *)(graph->ptr);
  GraphNode *graph_node1 = GraphGetNode(graphObject, c->argv[2]->ptr);
  GraphNode *graph_node2 = GraphGetNode(graphObject, c->argv[3]->ptr);

  // Return zero if any of the nodes is/are null
  if ((graph_node1 == NULL) || (graph_node2 == NULL)) {
    addReply(c, shared.nullbulk);
    return C_OK;
  }

  // Check whether the edge already exists
  edge = NULL;
  if (graph_node1 != NULL && graph_node2 != NULL)
    edge = GraphGetEdge(graphObject, graph_node1, graph_node2);

  if (edge != NULL) {
    addReplyDouble(c, edge->value);
  } else {
    addReply(c, shared.nullbulk);
  }
  return C_OK;
}

void gedgeCommand(client *c) {
  robj *graph;
  GraphEdge *edge;
  robj *key = c->argv[1];
  graph = lookupKeyRead(c->db, key);
  CHECK_GRAPH_EXISTS

  Graph *graphObject = (Graph *)(graph->ptr);

  if (equalStringObjects(c->argv[2], c->argv[3])) {
    addReply(c, shared.czero);
    return C_OK;
  }

  sds key1 = sdsnew(c->argv[2]->ptr);
  sds key2 = sdsnew(c->argv[3]->ptr);

  GraphNode *graph_node1 = GraphGetOrAddNode(graphObject, key1);
  GraphNode *graph_node2 = GraphGetOrAddNode(graphObject, key2);

  sdsfree(key1);
  sdsfree(key2);

  // Check whether the edge already exists
  edge = GraphGetEdge(graphObject, graph_node1, graph_node2);

  char *value_string = c->argv[4]->ptr;
  double value_double = atof(value_string);

  if (edge != NULL) {
    edge->value = value_double;
    addReplyLongLong(c, value_double);
    return C_OK;
  } else {
    edge = GraphEdgeCreate(graph_node1, graph_node2, value_double);
    GraphAddEdge(graphObject, edge);

    robj *value;
    addReplyDouble(c, value_double);
    return C_OK;
  }
}

void gnoderemCommand(client *c) {
  robj *graph;
  robj *edge_key;
  robj *key = c->argv[1];
  graph = lookupKeyRead(c->db, key);
  CHECK_GRAPH_EXISTS
  GraphEdge *edge;
  Graph *graphObject = (Graph *)(graph->ptr);
  GraphNode *node = GraphGetNode(graphObject, c->argv[2]->ptr);

  if (node) {
    GraphDeleteNode(graphObject, node);
    addReply(c, shared.cone);
  } else {
    addReply(c, shared.czero);
  }
}

void gedgeremCommand(client *c) {
  robj *graph;
  GraphEdge *edge;
  robj *key = c->argv[1];
  graph = lookupKeyRead(c->db, key);
  CHECK_GRAPH_EXISTS

  Graph *graphObject = (Graph *)(graph->ptr);

  GraphNode *graph_node1 = GraphGetNode(graphObject, c->argv[2]->ptr);
  GraphNode *graph_node2 = GraphGetNode(graphObject, c->argv[3]->ptr);

  // Check whether the edge already exists
  edge = GraphGetEdge(graphObject, graph_node1, graph_node2);

  if (edge) {
    GraphDeleteEdge(graphObject, edge);
    addReply(c, shared.cone);
  } else {
    addReply(c, shared.czero);
  }

  return C_OK;
}

void gedgeincrbyCommand(client *c) {
  robj *graph;
  robj *key = c->argv[1];
  graph = lookupKeyRead(c->db, key);
  CHECK_GRAPH_EXISTS
  GraphEdge *edge;
  Graph *graphObject = (Graph *)(graph->ptr);
  GraphNode *graph_node1 = GraphGetNode(graphObject, c->argv[2]->ptr);
  GraphNode *graph_node2 = GraphGetNode(graphObject, c->argv[3]->ptr);

  char *value_string = c->argv[4]->ptr;
  double value_double = atof(value_string);

  // Check whether the edge already exists
  edge = GraphGetEdge(graphObject, graph_node1, graph_node2);

  if (edge != NULL) {
    edge->value += value_double;
    addReplyDouble(c, edge->value);
    return C_OK;
  } else {
    edge = GraphEdgeCreate(graph_node1, graph_node2, value_double);
    GraphAddEdge(graphObject, edge);

    addReplyDouble(c, edge->value);
    return C_OK;
  }
}

void gnodesCommand(client *c) {
  robj *graph;
  robj *key = c->argv[1];
  sds pattern = c->argv[2]->ptr;
  int plen = sdslen(pattern), allkeys;
  graph = lookupKeyRead(c->db, key);
  CHECK_GRAPH_EXISTS

  Graph *graphObject = (Graph *)(graph->ptr);

  List *graphNodes = graphObject->nodes;
  ListNode *current_node = graphNodes->root;

  unsigned long numkeys = 0;
  void *replylen = addDeferredMultiBulkLength(c);

  allkeys = (pattern[0] == '*' && pattern[1] == '\0');

  while (current_node != NULL) {
    GraphNode *graphNode = (GraphNode *)(current_node->value);
    if (allkeys || stringmatchlen(pattern,plen,graphNode->key,sdslen(graphNode->key),0)) {
      addReplyBulk(c, createStringObject(graphNode->key, sdslen(graphNode->key)));
      numkeys += 1;
    }
    current_node = current_node->next;
  }

  setDeferredMultiBulkLength(c, replylen, numkeys);

  return C_OK;
}

void gedgesCommand(client *c) {
  robj *graph;
  robj *key = c->argv[1];
  graph = lookupKeyRead(c->db, key);
  CHECK_GRAPH_EXISTS

  Graph *graphObject = (Graph *)(graph->ptr);

  List *graphEdges = graphObject->edges;
  ListNode *current_node = graphEdges->root;

  int count = 0;
  while (current_node != NULL) {
    count++;
    current_node = current_node->next;
  }

  addReplyMultiBulkLen(c, count * 3);

  current_node = graphEdges->root;
  while (current_node != NULL) {
    GraphEdge *graphEdge = (GraphNode *)(current_node->value);
    robj *reply1 = createStringObject(graphEdge->node1->key, sdslen(graphEdge->node1->key));
    addReplyBulk(c, reply1);
    robj *reply2 = createStringObject(graphEdge->node2->key, sdslen(graphEdge->node2->key));
    addReplyBulk(c, reply2);
    robj *reply3 = createStringObjectFromLongDouble(graphEdge->value, 0);
    addReplyBulk(c, reply3);
    current_node = current_node->next;
  }

  return C_OK;
}
