//
//  StaticPrioStack.hpp
//
//  Copyright 2016 Mo DeJong.
//
//  See LICENSE for terms.
//
//  Statically allocated "priority queue" where the smallest
//  value is added and removed as a FIFO stack. This class
//  implements a doubly linked list priority stack where
//  the instance with the lowest prio is popped from the top
//  of a stack except that each value and node need not allocate
//  memory with new/delete for significant performance reasons.

#include "assert.h"

#include <string>
#include <sstream>

#include <vector>
#include <queue>
#include <set>

#if defined(DEBUG)
#include <iostream>
#endif // DEBUG

using namespace std;

// The vector size init determines how large the initial allocation is,
// this value can have a big impact on performance since avoiding the
// slow path of reallocating to enlarge the vector is critical.

#define ElemInitSize (4096 * 4)

// Each node is a double linked list is mapped to a fixed slot of memory
// at an offset. The type T must be a signed value since the init value is -1.

template <typename O>
class StaticPrioStackNode {
public:
  O prev;
  O next;
  
  StaticPrioStackNode()
  : prev(-1), next(-1)
  {
  }
};

typedef StaticPrioStackNode<int16_t> StaticPrioStackStdNode;

// The priority stack is used to push a value onto a stack defined
// for a specific priority level (0, N-1). The number of elements
// is known ahead of time, so that inserting a value of type T can
// be accomplished as a O(1) operation. Extracting the next element
// is also O(1).

// T is the type of the instance pushed onto each prio stack

template <class T>
class StaticPrioStack {
public:

  // Each prio is represented as a vector and there are N of these vectors.
  
  vector<vector<T> > elemTable;
  
  vector<StaticPrioStackStdNode> nodeTable;
  
  // HEAD node, access via head.next
  
  StaticPrioStackStdNode headNode;

  // Empty constructor
  
  StaticPrioStack()
  {
  }

  // Allocate structures to handle from (0, N-1) prio values
  
  void allocateN(int N) {
    elemTable.clear();
    elemTable.reserve(N);
    
    nodeTable.clear();
    nodeTable.reserve(N);
    
    // init each sub table with a fixed size
    
    for ( int prio = 0; prio < N; prio++ ) {
      vector<T> vec;
      vec.reserve(ElemInitSize);
      elemTable.push_back(std::move(vec));
      
      nodeTable.push_back(std::move(StaticPrioStackStdNode()));
    }
  }
  
  bool isEmpty() {
    return (headNode.next == -1);
  }
  
  int32_t head() {
    return headNode.next;
  }
  
  // Clear all entries from prio stacks
  
  void clear() {
    int N = (int) elemTable.size();
    
    for ( int prio = 0; prio < N; prio++ ) {
      vector<T> & elemVec = elemTable[prio];
      elemVec.clear();
      
      nodeTable[prio] = std::move(StaticPrioStackStdNode());
    }
    
    headNode.next = -1;
  }
  
  // Insert a node before the indicated prio slot, note that
  // this method depedns on there being an existing node to insert before.
  
  void insertNode(int prio, int insertBeforePrio) {
    const bool debug = false;
    if (debug) {
      printf("insertWaitListNode %d before %d\n", prio, insertBeforePrio);
    }
    
    StaticPrioStackStdNode & insertedNode = nodeTable[prio];
    
#if defined(DEBUG)
    assert(insertedNode.prev == -1);
    assert(insertedNode.next == -1);
#endif // DEBUG
    
    if (headNode.next == -1) {
      // No HEAD node is currently defined, optimized case
      
      headNode.next = prio;
      
      return;
    }
    
    // At this point, HEAD should be defined
    
#if defined(DEBUG)
    assert(headNode.next != -1);
#endif // DEBUG
    
    // It should not be possible for (err == insertBeforeErr)
    // at this point.
    
#if defined(DEBUG)
    assert(prio != insertBeforePrio);
#endif // DEBUG
    
    StaticPrioStackStdNode & insertBeforeNode = nodeTable[insertBeforePrio];
    
    if (insertBeforeNode.prev == -1) {
      // Inserting before current HEAD
      headNode.next = prio;
      
      if (debug) {
        printf("HEAD set to %d\n", prio);
      }
    } else {
      // Inserting before a non-HEAD node
      
      int prevOffset = insertBeforeNode.prev;
      
      StaticPrioStackStdNode & prevNode = nodeTable[prevOffset];
      
      if (debug) {
        printf("insert after %d and before %d\n", prevOffset, insertBeforePrio);
      }
      
      prevNode.next = prio;
    }
    
    insertedNode.prev = insertBeforeNode.prev;
    insertedNode.next = insertBeforePrio;
    
    insertBeforeNode.prev = prio;
  }

  // When a value must be appened to the end of the list, there is no
  // node to insert before, so this special case just appends to the
  // end of the linked list.
  
  void appendNode(int prio, int appendAfterPrio) {
    const bool debug = false;
    if (debug) {
      printf("appendNode %d after %d\n", prio, appendAfterPrio);
    }
    
#if defined(DEBUG)
    assert(prio != appendAfterPrio);
#endif // DEBUG
    
    StaticPrioStackStdNode & insertedNode = nodeTable[prio];
    
#if defined(DEBUG)
    assert(insertedNode.prev == -1);
    assert(insertedNode.next == -1);
#endif // DEBUG
    
    // Should not invoke append when HEAD indicates that list is empty
    
#if defined(DEBUG)
    assert(headNode.next != -1);
#endif // DEBUG
    
    // Append to end of list
    
    StaticPrioStackStdNode & appendAfterNode = nodeTable[appendAfterPrio];
    
#if defined(DEBUG)
    assert(appendAfterNode.next == -1);
#endif // DEBUG
    
    insertedNode.prev = appendAfterPrio;
    appendAfterNode.next = prio;
    
    if (debug) {
      printf("appened node for slot %d\n", prio);
    }
    
#if defined(DEBUG)
    assert(insertedNode.next == -1);
#endif // DEBUG
  }
  
  // When a specific node becomes empty, remove it from the wait list
  
  void unlinkNode(int prio) {
    const bool debug = false;
    if (debug) {
      printf("unlinkNode %d\n", prio);
    }
    
    StaticPrioStackStdNode & currentNode = nodeTable[prio];
    
#if defined(DEBUG)
    assert(prio != -1);
    assert(headNode.next != -1);
    
    vector<T> & elemVec = elemTable[prio];
    assert(elemVec.size() == 0);
#endif // DEBUG
    
    if (headNode.next == prio) {
      // This is the HEAD node
      
#if defined(DEBUG)
      assert(currentNode.prev == -1);
#endif // DEBUG
      
      headNode.next = currentNode.next;
    }
    
    // Update next back ref only if this is not the last node
    
    if (currentNode.next != -1) {
      StaticPrioStackStdNode & nextNode = nodeTable[currentNode.next];
      
      nextNode.prev = currentNode.prev;
    }
    
    // Update prev next ref only this is not the first node
    
    if (currentNode.prev != -1) {
      StaticPrioStackStdNode & prevNode = nodeTable[currentNode.prev];
      
      prevNode.next = currentNode.next;
    }
    
    // Set both prev and next to -1 for this slot
    
    currentNode = std::move(StaticPrioStackStdNode());
  }
  
  // FILO push to front of list for a specific prio
  
  void push(const T & elem, unsigned int prio) {
    const bool debug = false;
    
    if (debug) {
      printf("push prio %d\n", prio);
    }
    
#if defined(DEBUG)
    assert(elemTable.size() > 0);
    assert(prio < elemTable.size());
#endif // DEBUG
    
    vector<T> & elemVec = elemTable[prio];
    
    // Copy object contents to vector
    elemVec.push_back(elem);
    
    if (elemVec.size() == 1) {
      // When size of segment list goes from 0 to 1, insert the
      // segment into the wait list, locate insertion slot via
      // backward search.
      
      int prevPrio = (prio > 0) ? (prio - 1) : 0;
      
      if (debug) {
        printf("backwards table search for err smaller than %d starting at %d\n", prio, prevPrio);
      }
      
      // Note that insertAtErr can be -1 in the case where appeneding to the end of a list
      
      int insertAtPrio = 0;
      
      bool doSearch = true;
      
      bool doAppend = false;
      
      {
        // Check for case where the err for first element in list is larger (no search needed)
        // Note that the common case of 0 while HEAD is larger is handled here.
        
        auto headOffset = head();
        
        if (headOffset == -1) {
          // wait list is empty, search not needed
          doSearch = false;
        } else {
          // Look at offset for the HEAD, if it is larger than err then insert at front
          
          if (headOffset > prio) {
            if (debug) {
              printf("backwards search skipped since HEAD err %d, that is larger than %d\n", headOffset, prio);
            }
            
            insertAtPrio = headOffset;
            
            doSearch = false;
          }
        }
        
        // FIXME: could be impl as binary search
        // FIXME: would search forward by list eleme deref be faster?
        
        if (doSearch) {
          for ( ; prevPrio >= 0; prevPrio-- ) {
            vector<T> & elemVec = elemTable[prevPrio];
            
            if (elemVec.size() > 0) {
              if (debug) {
                printf("found entries for waitListErrTable[%d]\n", prevPrio);
              }
              
              const StaticPrioStackStdNode & prevNode = nodeTable[prevPrio];
              
              // Check for the special case of appending to the end of the list,
              // otherwise get insertAtErr by looking at the node after.
              
              if (prevNode.next == -1) {
                doAppend = true;
                insertAtPrio = prevPrio;
                
                if (debug) {
                  printf("found append to end case\n");
                }
              } else {
                insertAtPrio = prevNode.next;
                
                if (debug) {
                  printf("found insert before err slot %d\n", insertAtPrio);
                }
              }
              
              break;
            } else {
              if (debug) {
                printf("no entries at waitListErrTable[%d]\n", prevPrio);
              }
            }
          }
        }
      }
      
      if (doAppend) {
        appendNode(prio, insertAtPrio);
      } else {
        insertNode(prio, insertAtPrio);
      }
    }
    
    if (debug) {
      printf("push post add delta %d\n", prio);
      for ( int prio = head(); prio != -1; ) {
        StaticPrioStackStdNode & node = nodeTable[prio];
        vector<T> & elemVec = elemTable[prio];
        assert(elemVec.size() > 0);
        
        printf("prio: %d\n", prio);
        
        for ( auto it = elemVec.rbegin(); it != elemVec.rend(); it++ ) {
          const T & cd = *it;
          printf("elem %s\n", cd.toString().c_str());
        }
        
        prio = node.next;
      }
      printf("addToWaitList done\n");
    }
    
    return;
  }
  
  // Get the first element (the one with the smallest prio) as an O(1) op.
  
  T first(int * prioPtr) {
#if defined(DEBUG)
    assert(elemTable.size() > 0);
#endif // DEBUG
    
    if (isEmpty()) {
      *prioPtr = -1;
      // Note that a default empty constructor must be defined for T here
      return T();
    } else {
      int prio = head();
#if defined(DEBUG)
      assert(prio != -1);
#endif // DEBUG
      
      *prioPtr = prio;
      vector<T> & elemVec = elemTable[prio];
      
#if defined(DEBUG)
      assert(elemVec.size() >= 1);
#endif // DEBUG
      
      // Get the value at the end of the vector
      
      T elem = elemVec.back();
      elemVec.pop_back();
      
      // In the case where the last value with this specific err
      // then unlink the waitList entry.
      
      if (elemVec.size() == 0) {
        // Unlink current node from wait list
        
        unlinkNode(prio);
      }
      
      return elem;
    }
  }
  
  // Debug output of each elem
  
  string toString() const {
    stringstream s;
    
    int N = (int) elemTable.size();
    
    for ( int prio = 0; prio < N; prio++ ) {
      int N = (int) elemTable[prio].size();
      if (N == 0) {
        continue;
      }
      
      s << "[" << prio << "] " << endl;
      for ( T & elem : elemTable[prio] ) {
        s << elem;
      }
      s << endl;
    }
    
    return s.str();
  }
  
};

