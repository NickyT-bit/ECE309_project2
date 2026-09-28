# Design Log — Project 2

(500–800 words total. See spec §5 for what each section must cover.)

## Growth factor and amortized cost
My Conversation class uses a dynamically allocated array of Message objects. The array starts with a capacity of 1. When the array becomes full, append() allocates a new array with twice the previous capacity, copies the existing messages into the new array, deletes the old array, and then continues adding messages. For example, the capacities grow as 1, 2, 4, 8, 16, and so on.

The reason for doubling the capacity is to avoid allocating and copying the entire conversation every time a message is added. A single resize takes O(n) time because all existing messages have to be copied. However, the resizes happen less frequently as the array grows. Across n total appends, the total number of copied messages is bounded by a geometric series such as 1 + 2 + 4 + 8 + ... + n, which is O(n). Therefore, although some individual calls to append() take O(n), the total cost of n appends is O(n), giving append() an amortized cost of O(1).


## Rule of Five evidence

Rule of Five evidence implemented all five special member functions for Conversation. The destructor uses delete[] data_ to release the dynamically allocated array.

The copy constructor and copy assignment operator perform deep copies. They allocate a separate array using the other conversation's capacity and copy each existing Message. This means that changing one conversation does not change the other conversation. The copy assignment operator also checks for self-assignment before replacing the existing data.

The move constructor and move assignment operator transfer ownership of the dynamically allocated array instead of copying all the messages. They copy the source object's pointer, size, and capacity, and then set the source object's pointer to nullptr and its size and capacity to zero. This leaves the moved-from conversation valid and empty. The move assignment operator also deletes any existing array owned by the destination before taking ownership of the source array.

## Sentinel scanner: bounded pending_ proof

Sentinel scanner: bounded pending_ proof

The SentinelScanner uses the pending_ string to handle a sentinel that is split across multiple chunks. Each call to feed() combines pending_ with the new chunk and searches for the sentinel.

If the sentinel is found, the scanner returns the text before it and reports sentinel_found == true. If the sentinel is not found, the scanner returns most of the combined text but keeps only the last sentinel_.size() - 1 characters in pending_.

Keeping at most sentinel_.size() - 1 characters is sufficient because a sentinel cannot require more than that many previous characters to determine whether it continues into the next chunk. Therefore, the pending buffer is bounded by the size of the sentinel rather than the size of the entire conversation. For the required sentinel <|end_conversation|>, the scanner only needs to retain a fixed number of trailing characters between chunks. The flush() function returns any remaining pending text when input is finished.

## What I would change differently

If I were doing the project again, I would improve the testing and demonstration of the complete harness. The automated tests verify that the conversation, scanner, and stopping conditions work correctly, but successful tests do not print anything to the terminal. I would add a small demonstration that visibly shows several user and assistant turns and then demonstrates the sentinel stopping the conversation. This would make it easier to visually confirm that all of the individual components are working together, while keeping the existing assert-based tests for automated correctness.