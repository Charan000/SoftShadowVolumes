#pragma once

#include <assert.h>
#include <map>
#include <iostream>
#include <string>

namespace Utils
{
	//-----------------------------------------------------------------------------
	// Storage class	
	// Manages resources in the collection. One item for each resource.
	// Provides an interface with reference counting.
	//-----------------------------------------------------------------------------
	template < class Type, class Key = std::string, class Traits = std::less<Key> >  
	class Storage
	{
	public:
		// Reference type with ref. Counting
		struct resource
		{
			Type		*object;
			bool		copied;
			int		refCount;

            // Create & free
			resource(const Type& obj) : copied(true), refCount(1)
			{
				// Copy it
				object = new Type(obj);
			}
			resource(Type* obj) : object(obj), copied(false), refCount(1) 
			{
			}
		};

		// typedefs
		typedef					std::pair< Key, resource >				key_pair;
		typedef					std::map< Key, resource, Traits >		resource_map;
		typedef		typename	resource_map::iterator					resource_iterator;
		typedef					std::pair< resource_iterator, bool >	insert_pair;

		//-----------------------------------------------------------------------------
		// Handle class	
		// Provides interface to work with objects, stored in storage.
		// Valid tests are implemented in map::iterator class.
		//-----------------------------------------------------------------------------
		class handle
		{
		private:
			// location in the collection
			resource_iterator		location;

        public:
			// Create & free
			handle() : location( Storage::Instance()->items.end() ) {}
			explicit handle(const resource_iterator &location)
			{
				this->location = location;
			}
			handle(const handle& handle) : location(handle.location)
			{
				if (location != Storage::Instance()->items.end()) Storage::Instance()->AddReference(location);
			}
			~handle() 
			{ 
				if (location != Storage::Instance()->items.end()) Storage::Instance()->RemoveReference(location); 
			}

            // Get
			Type* Lock()
			{
				if (location->second.refCount == 1) return location->second.object;
				else return NULL;
			}

            const Type* GetObject() const
			{
				return location->second.object;
			}

            const Type& operator * () const
            {
                return *(location->second.object);
            }

			Key GetKey() const
			{
				return location->first;
			}

			// Operators
			const Type* operator -> () const
			{
				return location->second.object;
			}

            // Copy handle
			handle operator = (const handle& handle)
			{
				// We must not compare incompatible iterators
				if (location != handle.location)
				{
					if (location != Storage::Instance()->items.end()) Storage::Instance()->RemoveReference(location);
					location = handle.location;
					if (location != Storage::Instance()->items.end()) Storage::Instance()->AddReference(location);
				}
				return *this;
			}

            // Equal
    		bool operator == (const handle& handle) const
			{
				return (location == handle.location);
			}

			// Unequal
			bool operator != (const handle& handle) const
			{
				return (location != handle.location);
			}

			// Handle is valid?
			bool Exist() const
			{
				return location != Storage::Instance()->items.end();
			}

			// Destroy handle reference
			void Destroy()
			{
				Storage::Instance()->RemoveReference(location);
			}

			// Clone object
			// Because Storage don't know about key type, you must specify it yourself
			Type* Clone() const
			{
				return new Type(*location->second->object);
			}
		};

	protected:
		friend class handle;

        // Items
		resource_map			items;
		static	Storage*		instance;

        // Settings
		bool					autoRemoveItems;

		// Adds reference to item
		void AddReference(const resource_iterator &location)
		{
			location->second.refCount++;
		}

		// Removes reference
		void RemoveReference(resource_iterator &location)
		{
			location->second.refCount--;
			if ( location->second.refCount == 0 && autoRemoveItems ) 
			{
				delete location->second.object;
				items.erase(location);
			}
		}

	public:
		// Create & free
		Storage() : autoRemoveItems(true) {}
		~Storage() 
		{
			// Erase all objects, that won't be erased with list
			for(resource_iterator i = items.begin(); i != items.end(); ++i)
			{
				delete i->second.object;
			}
		}

		// Returns singleton instance
		static Storage* Instance()
		{
			if ( !instance ) instance = new Storage();
			return instance;
		}

		// Free singleton instance
		static void Free()
		{
			delete instance;
			instance = NULL;
		}

		// Add copy of resource in storage
		// If can't return handle of occupied key return end handle
		handle AddCopy(const Key& key, const Type& object)
		{
			resource		item(object);

            // Try to insert item
			insert_pair		insPair = items.insert( key_pair(key, item) );
			// Check and return
			if (insPair.second) return handle(insPair.first);
			else return End();
		}

		// Add copy of resource in storage
		// If can't return handle of occupied key return end handle
		handle Add(const Key& key, Type* object)
		{
			resource		item(object);
			// Try to insert item
			insert_pair		insPair = items.insert( key_pair(key, item) );
			// Check and return
			if (insPair.second) return handle(insPair.first);
			else return End();		
		}

		// Find specified object
		handle Get(const Key& key)
		{
			resource_iterator location = items.find(key);
			if ( location != items.end() ) AddReference(location);
			return handle(location);
		}

		// Return end
		handle End()
		{
			return handle( items.end() );
		}

        // Same
		handle operator[](const Key& key)
		{
			return handle( items.find(key) );
		}
	};

	// Implementation
	template<class T, class Key, class Traits>
	Storage<T, Key, Traits>* Storage<T, Key, Traits>::instance;
}
