#include <stdio.h>
#include <cstdlib>
#include <cassert>
#include <vector>
#include <stack>
#include <unordered_map>

template <typename T> 
struct arena {
  arena(size_t size) {
    m_data.resize(size);
    for (size_t i = 0; i < size; i++) {
      m_freeList.push(m_data.data() + i);
    }
  }

  T *insert(T &&item) {
    T *ptr = nullptr;

    if (!m_freeList.empty()) {
      // allocate from freelist
      ptr = m_freeList.top();
      m_freeList.pop();
      *ptr = item;
    } else {
      // allocate new space
      m_data.push_back(std::forward<T>(item));
      ptr = &m_data.back();
    }
    return ptr;
  }

  void remove(T *ptr) { m_freeList.push(ptr); }

  std::vector<T> m_data;
  std::stack<T *> m_freeList;
};

int toSide(char side) {
  if (side == 'b')
    return 1;
  else if (side == 's')
    return -1;
  assert(false);
  return 0;
}

struct Order {
  int id;
  int size;
  double price;
  Order *next;
  Order *prev;

  Order(int id, int size, double price)
      : id(id), size(size), price(price), next(nullptr), prev(nullptr) {}

  Order() {}
  
  bool isBuy() const {
      return size>0;
  }
};

template<typename T>
struct LList {
    T *head, *tail;
    
    LList() 
    :   head(nullptr),
        tail(nullptr) {}
    
    void push_back(T *item) {
        if (head == nullptr) {
            head = tail = item;
        } else {
            // should be sorted-insert here, not done so bug
            tail->next = item;
            item->prev = tail;
            tail = item;
        }
    }

    void push_front(T *item) {
        item->next = head;
        head = item;
        if(tail==nullptr)
            tail = head;
    }
    void insert_before(T *node, T *item) {
        if(node==nullptr)
            push_back(item); // to tail
        else  if(node->prev==nullptr) {
            push_front(item);
        }else {
            node->prev->next = item;
            item->next = node->next;
        }
    }

    template<typename Pred>
    T* find_first(Pred pred) {
      T* node = head;
      while(node!=nullptr && !pred(node)) {
          node = node->next;
      }
      return node;
    }

    void remove(T *item) {
      if (item->prev != nullptr) {
        item->prev->next = item->next;
        if (item->next == nullptr)
          tail = item->prev;
      } else { // head
        head = item->next;
        if (item->next == nullptr) {
          tail = nullptr;
        }
      }
    }

    bool empty() const { return head == nullptr; }
};

struct Level {
  double price;
  int count;
  int size;
  Level *next;
  Level *prev;
  LList<Order> queue;

  Level(double price)
      : price(price), count(0), size(0),  next(nullptr), prev(nullptr) {}
  
  void insert(Order *ord) {
      queue.push_back(ord);
      count++;
      size+=ord->size;
  }

  void remove(Order *ord) {
    queue.remove(ord);
    count--;
    size-=ord->size;
  }

  bool empty() const {
      return queue.empty();
  }
};

template<int Side>
struct LevelsIndex {
  using Map = std::unordered_map<double, Level>;
  Map ht;
  LList<Level> lst;


  //void insert(Level&& lvl) {
  //  auto itr = ht.insert(Map::value_type(lvl.price, std::move(lvl));
  //  lst.insert(&itr->second); 
  //}
  
  void remove(Level *lvl) {
    auto itr = ht.find(lvl->price);
    lst.remove(lvl);
    if(best==lvl) {
        best = best->next;
    }
    ht.erase(itr);
  }

  Level *operator[](double price) {
    auto itr = ht.find(price);
    if (itr == ht.end()) {
      Level *lvl = &ht.emplace(price, price).first->second;
      // new level
      Level *node = lst.find_first([&](Level *n){
          return Side*(n->price-lvl->price)<=0;});
      lst.insert_before(node, lvl);
      if(best==nullptr || Side*(lvl->price-best->price)>0)
        best = lvl;
      return lvl;
    } else {
      return &itr->second;
    }
  }

  size_t size() const {
      return ht.size();
  }

  Level* at(int index) {
    auto result = best;
    assert(index<size());
    for(size_t i=0; i<index; i++) {
        assert(result!=nullptr);
        result = result->next;
    }
    return result;
  }
  Level *best = nullptr;
};

/* 
    OrderBook is a level 3 orderbook.  Please fill out the stub below.
*/
class OrderBook{
public:
    OrderBook(size_t size = 1024)
        : m_orders(size) {
        
    }

    //adds order to order book
    void newOrder(int orderId, char side, int size, double price){
      auto ord = m_orders.insert(Order(orderId, toSide(side) * size, price));
      // get level
      Level *lvl = byOrder(ord);
      // link tail
      lvl->insert(ord);
      m_idmap.emplace(ord->id, ord);
    }

    //changes quantity contained in order
    void changeOrder(int orderId, int newSize){
        Order *ord = byId(orderId);
        byOrder(ord)->size += newSize - ord->size;
        ord->size = newSize;
    }

    //replaces order with different order
    void replaceOrder(int oldOrderId, int orderId, char side, int size, double price){
        deleteOrder(oldOrderId);
        newOrder(orderId, side, size, price);
    }

    //deletes order from orderbook
    void deleteOrder(int orderId){
        Order *ord = popById(orderId);
        Level *lvl = byOrder(ord);
        lvl->remove(ord);
        m_orders.remove(ord);
        if(lvl->empty()) {
            if(ord->isBuy())
                m_buys.remove(lvl);
            else
                m_sells.remove(lvl);
        }
    }

    //returns the number of levels on the side
    int getNumLevels(char side){
        switch(toSide(side)) {
            case 1: return m_buys.size();
            case -1: return m_sells.size();
            default: assert(false); return 0;
        }
    }
    //returns the price of a level.  Level 0 is top of book.
    double getLevelPrice(char side, int level){
      return byLevel(level, toSide(side))->price;
    }

    //returns the size of a level.
    int getLevelSize(char side, int level){
      auto lvl = byLevel(level, toSide(side));
      return std::abs(lvl->size);
    }

    //returns the number of orders contained in price level.
    int getLevelOrderCount(char side, int level){
      return byLevel(level, toSide(side))->count;
    }

protected:
  Order *byId(int orderId) {
    auto itr = m_idmap.find(orderId);
    assert(itr != m_idmap.end());
    return itr->second;
  }
  Order *popById(int orderId) {
    auto itr = m_idmap.find(orderId);
    assert(itr!=m_idmap.end());
    m_idmap.erase(itr);
    return itr->second;
  }

  Level *byOrder(const Order *ord) {
    if(ord->isBuy())
        return m_buys[ord->price];
    else 
        return m_sells[ord->price];
  }

  Level *byLevel(int level, int side) {
    return side>0 ? m_buys.at(level) : m_sells.at(level);
  }
  using IdMap =  std::unordered_map<int, Order *>;
  IdMap m_idmap;
  arena<Order> m_orders;
  LevelsIndex<1> m_buys;
  LevelsIndex<-1> m_sells;
};

/*
    Do not change main function
*/

int main(int argc, char* argv[]){
  char instruction;
  char side;
  int orderId;
  double price;
  int size;
  int oldOrderId;

  FILE * file = stdout;

  if(argc<2) {
      fprintf(stderr, "Usage: %s test_cases/input001.txt\n", argv[0]);
      return -1;
  }
  
  FILE* input_file = fopen(argv[1], "rt");

  OrderBook book;

  while(fscanf(input_file, "%c %c %d %d %lf %d\n", &instruction, &side, &orderId, &size, &price, &oldOrderId)!=EOF){
    switch(instruction){
      case 'n':
        book.newOrder(orderId,side,size,price);
        break;
      case 'u':
        book.changeOrder(orderId,size);
        break;
      case 'r':
        book.replaceOrder(oldOrderId,orderId,side,size,price);
        break;
      case 'd':
        book.deleteOrder(orderId);
        break;
      case 'p':
        fprintf(file,"%.2lf\n",book.getLevelPrice(side,orderId));
        break;
      case 's':
        fprintf(file,"%d\n",book.getLevelSize(side,orderId));
        break;
      case 'l':
        fprintf(file,"%d\n",book.getNumLevels(side));
        break;
      case 'c':
        fprintf(file,"%d\n",book.getLevelOrderCount(side,orderId));
        break;
      default:
        fprintf(file,"invalid input\n");
    }
  }

  fclose(file);
  return 0;
}
