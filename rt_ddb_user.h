#ifndef CDB_USER_H
#define CDB_USER_H

#include <rtr.h>
#include <vector>
#include <string>

/*
 * A subscription should be unique by the tag_name. If a subscription with a given tag name exists,
 * we should add any further subscriptions to this tag_name by adding an address or by adding a tag.
 * Also all of the tags within this subscription name should have the same tag type.
 * 
 * Note: for now, one subscription corresponds to one tag / address pair.
 */
class remote_subscription
{
	public:	
		//create a remote subscription: note - tag is NULL and must be initialized depending on the type 
		//of the first received update for its value - this may be a problem if we try to depend on the value
		//too early ...may need to investigate more
		remote_subscription(std::string name) {tag_name = name; tag = NULL; };
		void add_address(std::string address) {addresses.push_back(address);};
		void add_tag(m_tag * t) {tag = t;};
		std::vector<std::string> get_addresses(void) { return addresses; };
		m_tag * get_tag(void) {return tag;};
		std::string get_tag_name(void){return tag_name;};
		~remote_subscription();
	protected:
	private:
		std::string tag_name;
		std::string remote_identifier;
		std::vector<std::string> addresses;
		m_tag * tag;
};

std::vector<remote_subscription *> subscribe_broadcast(std::string name, int timeout_sec);
std::vector<m_tag *> get_tag_broadcast(std::string name, int timeout_sec);

#endif