# Highlights

1. Use arena with freelist implemented as stack to store orders
2. Use standrad hashmap (assuming it will not allocate often) to track price->level
3. Use linked lists to chain orders in level and chain levels in orderbook
4. Keep best bid and best offer levels 
