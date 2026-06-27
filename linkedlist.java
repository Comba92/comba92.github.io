class LinkedList<T> {
  class Node<T> {
    T val;
    Node<T> next;

    public Node(T val) {
      this(val, null);
    }

    public Node(T val, Node<T> next) {
      this.val = val;
      this.next = next;
    }
  }

  private Node<T> head;
  // puntatore alla coda può salvare delle iterazioni sulla lista
  // private Node<T> tail;
  private int size;

  public LinkedList() {}

  public int size() { return this.size; }
  public boolean isEmpty() { return this.head == null; }

  public Node<T> front() {
    return this.head;
  }

  // volendo, per comodità, possiamo salvare un puntatore alla coda come membro, in modo da non iterare tutta la lista
  public Node<T> back() {
    Node it = this.head;
    while (it.next != null) it = it.next;
    return it;
  }

  public void pushFront(T val) {
    var newHead = new Node(val, this.head);
    this.head = newHead;
    this.size += 1;
  }

  public void pushBack(T val) {
    if (isEmpty()) {
      pushFront(val);
      return;
    }

    var newBack = new Node(val);
    back().next = newBack;
    this.size += 1;
  }

  public T popFront() {
    assert !isEmpty();

    var ret = this.head;
    this.head = this.head.next;
    this.size -= 1;
    return ret.val;
  }

  public T popBack() {
    assert !isEmpty();
    return removeAt(this.size-1);
  }

  public Node<T> get(int idx) {
    var it = this.head;
    for (int i=0; i<idx && it != null; i++) {
      it = it.next;
    }

    assert it != null;
    return it;
  }

  public void insertAt(int idx, T val) {
    var it = get(idx);
    var next = it.next;

    var newNode = new Node(val, next);
    it.next = newNode;
    this.size += 1;
  }

  public boolean contains(T val) {
    var it = this.head;
    while (it != null) {
      if (it.val == val) return true;
      it = it.next;
    }

    return false;
  }

  public T removeAt(int idx) {
    if (idx == 0) {
      return popFront();
    }

    var prev = get(idx-1);
    var it = prev.next;
    prev.next = it.next;

    this.size -= 1;
    return it.val;
  }

  public int removeAll(T val) {
    int count = 0;

    while (this.head.val == val) {
      popFront();
      count += 1;
    }
    
    if (isEmpty()) {
      this.size -= count;
      return count;
    }

    // abbiamo gestito il caso della testa
    // invariante: siamo sicuri ora che prev non è mai da rimuovere
    // controlliamo solo next
    var prev = this.head;
    var next = prev.next;
    while (next != null) {
      // trovato, elimino nodo senza avanzare
      // prossima iterazione controllero il nuovo
      if (next.val == val) {
        prev.next = next.next;

        next = prev.next;
        count += 1;
      } else {
        // avanziamo entrambi i puntatori
        prev = prev.next;
        next = next.next;
      }
    }

    this.size -= count;
    return count;
  }
}