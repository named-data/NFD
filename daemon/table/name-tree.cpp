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
NameTree::findLongestPrefixMatch(const Name& prefix, const name_tree::EntrySelector& entrySelector)
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

shared_ptr<std::vector<shared_ptr<name_tree::Entry> > >
NameTree::fullEnumerate(const name_tree::EntrySelector& entrySelector)
{
  NFD_LOG_DEBUG("fullEnumerate");

  shared_ptr<std::vector<shared_ptr<name_tree::Entry> > > results =
    make_shared<std::vector<shared_ptr<name_tree::Entry> > >();

  for (size_t i = 0; i < m_nBuckets; i++) {
    for (name_tree::Node* node = m_buckets[i]; node != 0; node = node->m_next) {
      if (static_cast<bool>(node->m_entry) && entrySelector(*node->m_entry)) {
        results->push_back(node->m_entry);
      }
    }
  }

  return results;
}

// Helper function for partialEnumerate()
void
NameTree::partialEnumerateAddChildren(shared_ptr<name_tree::Entry> entry,
                                      const name_tree::EntrySelector& entrySelector,
                                      std::vector<shared_ptr<name_tree::Entry> >& results)
{
  BOOST_ASSERT(static_cast<bool>(entry));
  
  if (!entrySelector(*entry)) {
    return;
  }

  results.push_back(entry);
  for (size_t i = 0; i < entry->m_children.size(); i++)
    {
      shared_ptr<name_tree::Entry> child = entry->m_children[i];
      partialEnumerateAddChildren(child, entrySelector, results);
    }
}

shared_ptr<std::vector<shared_ptr<name_tree::Entry> > >
NameTree::partialEnumerate(const Name& prefix,
                           const name_tree::EntrySelector& entrySelector)
{
  NFD_LOG_DEBUG("partialEnumerate" << prefix);

  shared_ptr<std::vector<shared_ptr<name_tree::Entry> > > results =
    make_shared<std::vector<shared_ptr<name_tree::Entry> > >();

  // find the hash bucket corresponding to that prefix
  shared_ptr<name_tree::Entry> entry = findExactMatch(prefix);

  if (static_cast<bool>(entry)) {
    // go through its children list via depth-first-search
    partialEnumerateAddChildren(entry, entrySelector, *results);
  }

  return results;
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
NameTree::dump(std::ostream& output)
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

} // namespace nfd
