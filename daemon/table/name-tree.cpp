/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/**
 * Copyright (C) 2014 Named Data Networking Project
 * See COPYING for copyright and distribution information.
 */

// Name Tree (Name Prefix Hash Table)

#include "name-tree.hpp"
#include "core/logger.hpp"

namespace nfd {

NFD_LOG_INIT("NameTree");

namespace name_tree {

// Interface of different hash functions
uint32_t
hashName(const Name& prefix)
{
  // fixed value. Used for debugging only.
  uint32_t ret = 0;

  // Boost hash
  // requires the /boost/functional/hash.hpp header file
  // TODO: improve hash efficiency with Name type
  boost::hash<std::string> stringHash;
  ret = stringHash(prefix.toUri());

  return ret;
}

} // namespace name_tree

NameTree::NameTree(size_t nBuckets)
  : m_nItems(0)
  , m_nBuckets(nBuckets)
  , m_loadFactor(0.5)
  , m_resizeFactor(2)
  , m_endIterator(FULL_ENUMERATE_TYPE, *this, m_end)
{
  m_resizeThreshold = static_cast<size_t>(m_loadFactor *
                                          static_cast<double>(m_nBuckets));

  // array of node pointers
  m_buckets = new name_tree::Node*[m_nBuckets];
  // Initialize the pointer array
  for (size_t i = 0; i < m_nBuckets; i++)
    m_buckets[i] = 0;
}

NameTree::~NameTree()
{
  for (size_t i = 0; i < m_nBuckets; i++)
    {
      if (m_buckets[i] != 0)
        delete m_buckets[i];
    }

  delete [] m_buckets;
}

// insert() is a private function, and called by only lookup()
std::pair<shared_ptr<name_tree::Entry>, bool>
NameTree::insert(const Name& prefix)
{
  NFD_LOG_DEBUG("insert " << prefix);

  uint32_t hashValue = name_tree::hashName(prefix);
  uint32_t loc = hashValue % m_nBuckets;

  NFD_LOG_DEBUG("Name " << prefix << " hash value = " << hashValue << "  location = " << loc);

  // Check if this Name has been stored
  name_tree::Node* node = m_buckets[loc];
  name_tree::Node* nodePrev = node;  // initialize nodePrev to node

  for (node = m_buckets[loc]; node != 0; node = node->m_next)
    {
      if (static_cast<bool>(node->m_entry))
        {
          if (prefix == node->m_entry->m_prefix)
            {
              return std::make_pair(node->m_entry, false); // false: old entry
            }
        }
      nodePrev = node;
    }

  NFD_LOG_DEBUG("Did not find " << prefix << ", need to insert it to the table");

  // If no bucket is empty occupied, we need to create a new node, and it is
  // linked from nodePrev
  node = new name_tree::Node();
  node->m_prev = nodePrev;

  if (nodePrev == 0)
    {
      m_buckets[loc] = node;
    }
  else
    {
      nodePrev->m_next = node;
    }

  // Create a new Entry
  shared_ptr<name_tree::Entry> entry(make_shared<name_tree::Entry>(prefix));
  entry->setHash(hashValue);
  node->m_entry = entry; // link the Entry to its Node
  entry->m_node = node; // link the node to Entry. Used in eraseEntryIfEmpty.

  return std::make_pair(entry, true); // true: new entry
}

// Name Prefix Lookup. Create Name Tree Entry if not found
shared_ptr<name_tree::Entry>
NameTree::lookup(const Name& prefix)
{
  NFD_LOG_DEBUG("lookup " << prefix);

  shared_ptr<name_tree::Entry> entry;
  shared_ptr<name_tree::Entry> parent;

  for (size_t i = 0; i <= prefix.size(); i++)
    {
      Name temp = prefix.getPrefix(i);

      // insert() will create the entry if it does not exist.
      std::pair<shared_ptr<name_tree::Entry>, bool> ret = insert(temp);
      entry = ret.first;

      if (ret.second == true)
        {
          m_nItems++; /* Increase the counter */
          entry->m_parent = parent;

          if (static_cast<bool>(parent))
            {
              parent->m_children.push_back(entry);
            }
        }

      if (m_nItems > m_resizeThreshold)
        {
          resize(m_resizeFactor * m_nBuckets);
        }

      parent = entry;
    }
  return entry;
}

// Exact Match
shared_ptr<name_tree::Entry>
NameTree::findExactMatch(const Name& prefix) const
{
  NFD_LOG_DEBUG("findExactMatch " << prefix);

  uint32_t hashValue = name_tree::hashName(prefix);
  uint32_t loc = hashValue % m_nBuckets;

  NFD_LOG_DEBUG("Name " << prefix << " hash value = " << hashValue <<
                "  location = " << loc);

  shared_ptr<name_tree::Entry> entry;
  name_tree::Node* node = 0;

  for (node = m_buckets[loc]; node != 0; node = node->m_next)
    {
      entry = node->m_entry;
      if (static_cast<bool>(entry))
        {
          if (hashValue == entry->getHash() && prefix == entry->getPrefix())
            {
              return entry;
            }
        } // if entry
    } // for node

  // if not found, the default value of entry (null pointer) will be returned
  entry.reset();
  return entry;
}

// Longest Prefix Match
// Return the longest matching Entry address
// start from the full name, and then remove 1 name comp each time
shared_ptr<name_tree::Entry>
NameTree::findLongestPrefixMatch(const Name& prefix, const name_tree::EntrySelector& entrySelector) const
{
  NFD_LOG_DEBUG("findLongestPrefixMatch " << prefix);

  shared_ptr<name_tree::Entry> entry;

  for (int i = prefix.size(); i >= 0; i--)
    {
      entry = findExactMatch(prefix.getPrefix(i));
      if (static_cast<bool>(entry) && entrySelector(*entry))
        return entry;
    }

  return shared_ptr<name_tree::Entry>();
}

// return {false: this entry is not empty, true: this entry is empty and erased}
bool
NameTree::eraseEntryIfEmpty(shared_ptr<name_tree::Entry> entry)
{
  BOOST_ASSERT(static_cast<bool>(entry));

  NFD_LOG_DEBUG("eraseEntryIfEmpty " << entry->getPrefix());

  // first check if this Entry can be erased
  if (entry->isEmpty())
    {
      // update child-related info in the parent
      shared_ptr<name_tree::Entry> parent = entry->getParent();

      if (static_cast<bool>(parent))
        {
          std::vector<shared_ptr<name_tree::Entry> >& parentChildrenList =
            parent->getChildren();

          bool isFound = false;
          size_t size = parentChildrenList.size();
          for (size_t i = 0; i < size; i++)
            {
              if (parentChildrenList[i] == entry)
                {
                  parentChildrenList[i] = parentChildrenList[size - 1];
                  parentChildrenList.pop_back();
                  isFound = true;
                  break;
                }
            }

          BOOST_ASSERT(isFound == true);
        }

      // remove this Entry and its Name Tree Node
      name_tree::Node* node = entry->m_node;
      name_tree::Node* nodePrev = node->m_prev;

      // configure the previous node
      if (nodePrev != 0)
        {
          // link the previous node to the next node
          nodePrev->m_next = node->m_next;
        }
      else
        {
          m_buckets[entry->getHash() % m_nBuckets] = node->m_next;
        }

      // link the previous node with the next node (skip the erased one)
      if (node->m_next != 0)
        {
          node->m_next->m_prev = nodePrev;
          node->m_next = 0;
        }

      BOOST_ASSERT(node->m_next == 0);

      m_nItems--;
      delete node;

      if (static_cast<bool>(parent))
        eraseEntryIfEmpty(parent);

      return true;

    } // if this entry is empty

  return false; // if this entry is not empty
}

NameTree::const_iterator
NameTree::fullEnumerate(const name_tree::EntrySelector& entrySelector) const
{
  NFD_LOG_DEBUG("fullEnumerate");

  // find the first eligible entry
  for (size_t i = 0; i < m_nBuckets; i++)
    {
      for (name_tree::Node* node = m_buckets[i]; node != 0; node = node->m_next)
        {
          if (static_cast<bool>(node->m_entry) && entrySelector(*node->m_entry))
            {
              const_iterator it(FULL_ENUMERATE_TYPE, *this, node->m_entry, entrySelector);
              return it;
            }
        }
    }

  // If none of the entry satisfies the requirements, then return the end() iterator.
  return end();
}

NameTree::const_iterator
NameTree::partialEnumerate(const Name& prefix,
                           const name_tree::EntrySubTreeSelector& entrySubTreeSelector) const
{
  // the first step is to process the root node
  shared_ptr<name_tree::Entry> entry = findExactMatch(prefix);
  if (!static_cast<bool>(entry))
    {
      return end();
    }

  std::pair<bool, bool>result = entrySubTreeSelector(*entry);
  const_iterator it(PARTIAL_ENUMERATE_TYPE,
                    *this,
                    entry,
                    name_tree::AnyEntry(),
                    entrySubTreeSelector);

  it.m_shouldVisitChildren = (result.second && entry->hasChildren());

  if (result.first)
    {
      // root node is acceptable
      return it;
    }
  else
    {
      // let the ++ operator handle it
      ++it;
      return it;
    }
}

NameTree::const_iterator
NameTree::findAllMatches(const Name& prefix,
                         const name_tree::EntrySelector& entrySelector) const
{
  NFD_LOG_DEBUG("NameTree::findAllMatches" << prefix);

  // As we are using Name Prefix Hash Table, and the current LPM() is
  // implemented as starting from full name, and reduce the number of
  // components by 1 each time, we could use it here.
  // For trie-like design, it could be more efficient by walking down the
  // trie from the root node.

  shared_ptr<name_tree::Entry> entry = findLongestPrefixMatch(prefix, entrySelector);

  if (static_cast<bool>(entry))
    {
      const_iterator it(FIND_ALL_MATCHES_TYPE, *this, entry, entrySelector);
      return it;
    }
  // If none of the entry satisfies the requirements, then return the end() iterator.
  return end();
}

// Hash Table Resize
void
NameTree::resize(size_t newNBuckets)
{
  NFD_LOG_DEBUG("resize");

  name_tree::Node** newBuckets = new name_tree::Node*[newNBuckets];
  size_t count = 0;

  // referenced ccnx hashtb.c hashtb_rehash()
  name_tree::Node** pp = 0;
  name_tree::Node* p = 0;
  name_tree::Node* pre = 0;
  name_tree::Node* q = 0; // record p->m_next
  size_t i;
  uint32_t h;
  uint32_t b;

  for (i = 0; i < newNBuckets; i++)
    {
      newBuckets[i] = 0;
    }

  for (i = 0; i < m_nBuckets; i++)
    {
      for (p = m_buckets[i]; p != 0; p = q)
        {
          count++;
          q = p->m_next;
          BOOST_ASSERT(static_cast<bool>(p->m_entry));
          h = p->m_entry->m_hash;
          b = h % newNBuckets;
          for (pp = &newBuckets[b]; *pp != 0; pp = &((*pp)->m_next))
            {
              pre = *pp;
              continue;
            }
          p->m_prev = pre;
          p->m_next = *pp; // Actually *pp always == 0 in this case
          *pp = p;
        }
    }

  BOOST_ASSERT(count == m_nItems);

  name_tree::Node** oldBuckets = m_buckets;
  m_buckets = newBuckets;
  delete oldBuckets;

  m_nBuckets = newNBuckets;
  m_resizeThreshold = (int)(m_loadFactor * (double)m_nBuckets);
}

// For debugging
void
NameTree::dump(std::ostream& output) const
{
  NFD_LOG_DEBUG("dump()");

  name_tree::Node* node = 0;
  shared_ptr<name_tree::Entry> entry;

  using std::endl;

  for (size_t i = 0; i < m_nBuckets; i++)
    {
      for (node = m_buckets[i]; node != 0; node = node->m_next)
        {
          entry = node->m_entry;

          // if the Entry exist, dump its information
          if (static_cast<bool>(entry))
            {
              output << "Bucket" << i << "\t" << entry->m_prefix.toUri() << endl;
              output << "\t\tHash " << entry->m_hash << endl;

              if (static_cast<bool>(entry->m_parent))
                {
                  output << "\t\tparent->" << entry->m_parent->m_prefix.toUri();
                }
              else
                {
                  output << "\t\tROOT";
                }
              output << endl;

              if (entry->m_children.size() != 0)
                {
                  output << "\t\tchildren = " << entry->m_children.size() << endl;

                  for (size_t j = 0; j < entry->m_children.size(); j++)
                    {
                      output << "\t\t\tChild " << j << " " <<
                        entry->m_children[j]->getPrefix() << endl;
                    }
                }

            } // if (static_cast<bool>(entry))

        } // for node
    } // for int i

  output << "Bucket count = " << m_nBuckets << endl;
  output << "Stored item = " << m_nItems << endl;
  output << "--------------------------\n";
}

NameTree::const_iterator::const_iterator(NameTree::IteratorType type,
                            const NameTree& nameTree,
                            shared_ptr<name_tree::Entry> entry,
                            const name_tree::EntrySelector& entrySelector,
                            const name_tree::EntrySubTreeSelector& entrySubTreeSelector)
  : m_nameTree(nameTree)
  , m_entry(entry)
  , m_subTreeRoot(entry)
  , m_entrySelector(make_shared<name_tree::EntrySelector>(entrySelector))
  , m_entrySubTreeSelector(make_shared<name_tree::EntrySubTreeSelector>(entrySubTreeSelector))
  , m_type(type)
  , m_shouldVisitChildren(true)
{
}

// operator++()
NameTree::const_iterator
NameTree::const_iterator::operator++()
{
  NFD_LOG_DEBUG("const_iterator::operator++()");

  BOOST_ASSERT(m_entry != m_nameTree.m_end);

  if (m_type == FULL_ENUMERATE_TYPE) // fullEnumerate
    {
      bool isFound = false;
      // process the entries in the same bucket first
      while (m_entry->m_node->m_next != 0)
        {
          m_entry = m_entry->m_node->m_next->m_entry;
          if ((*m_entrySelector)(*m_entry))
            {
              isFound = true;
              return *this;
            }
        }

      // process other buckets

      for (int newLocation = m_entry->m_hash % m_nameTree.m_nBuckets + 1;
           newLocation < static_cast<int>(m_nameTree.m_nBuckets);
           ++newLocation)
        {
          // process each bucket
          name_tree::Node* node = m_nameTree.m_buckets[newLocation];
          while (node != 0)
            {
              m_entry = node->m_entry;
              if ((*m_entrySelector)(*m_entry))
                {
                  isFound = true;
                  return *this;
                }
              node = node->m_next;
            }
        }
      BOOST_ASSERT(isFound == false);
      // Reach to the end()
      m_entry = m_nameTree.m_end;
      return *this;
    }

  if (m_type == PARTIAL_ENUMERATE_TYPE) // partialEnumerate
    {
      // We use pre-order traversal.
      // if at the root, it could have already been accepted, or this
      // iterator was just declared, and root doesn't satisfy the
      // requirement
      // The if() section handles this special case
      // Essentially, we need to check root's fist child, and the rest will
      // be the same as normal process
      if (m_entry == m_subTreeRoot)
        {
          if (m_shouldVisitChildren)
            {
              m_entry = m_entry->getChildren()[0];
              std::pair<bool, bool> result = ((*m_entrySubTreeSelector)(*m_entry));
              m_shouldVisitChildren = (result.second && m_entry->hasChildren());
              if(result.first)
                {
                  return *this;
                }
              else
                {
                  // the first child did not meet the requirement
                  // the rest of the process can just fall through the while loop
                  // as normal
                }
            }
          else
            {
              // no children, should return end();
              // just fall through
            }
        }

      // The first thing to do is to visit its child, or go to find its possible
      // siblings
      while (m_entry != m_subTreeRoot)
        {
          if (m_shouldVisitChildren)
            {
              // If this subtree should be visited
              m_entry = m_entry->getChildren()[0];
              std::pair<bool, bool> result = ((*m_entrySubTreeSelector)(*m_entry));
              m_shouldVisitChildren = (result.second && m_entry->hasChildren());
              if (result.first) // if this node is acceptable
                {
                  return *this;
                }
              else
                {
                  // do nothing, as this node is essentially ignored
                  // send this node to the while loop.
                }
            }
          else
            {
              // Should try to find its sibling
              shared_ptr<name_tree::Entry> parent = m_entry->getParent();

              std::vector<shared_ptr<name_tree::Entry> >& parentChildrenList = parent->getChildren();
              bool isFound = false;
              size_t i = 0;
              for (i = 0; i < parentChildrenList.size(); i++)
                {
                  if (parentChildrenList[i] == m_entry)
                    {
                      isFound = true;
                      break;
                    }
                }

              BOOST_ASSERT(isFound == true);
              if (i < parentChildrenList.size() - 1) // m_entry not the last child
                {
                  m_entry = parentChildrenList[i + 1];
                  std::pair<bool, bool> result = ((*m_entrySubTreeSelector)(*m_entry));
                  m_shouldVisitChildren = (result.second && m_entry->hasChildren());
                  if (result.first) // if this node is acceptable
                    {
                      return *this;
                    }
                  else
                    {
                      // do nothing, as this node is essentially ignored
                      // send this node to the while loop.
                    }
                }
              else
                {
                  // m_entry is the last child, no more sibling, should try to find parent's sibling
                  m_shouldVisitChildren = false;
                  m_entry = parent;
                }
            }
        }

      m_entry = m_nameTree.m_end;
      return *this;
    }

  if (m_type == FIND_ALL_MATCHES_TYPE) // findAllMatches
    {
      // Assumption: at the beginning, m_entry was initialized with the first
      // eligible Name Tree entry (i.e., has a PIT entry that can be satisfied
      // by the Data packet)

      while (static_cast<bool>(m_entry->getParent()))
        {
          m_entry = m_entry->getParent();
          if ((*m_entrySelector)(*m_entry))
            return *this;
        }

      // Reach to the end (Root)
      m_entry = m_nameTree.m_end;
      return *this;
    }

  BOOST_ASSERT(false); // unknown type
  return *this;
}

} // namespace nfd
