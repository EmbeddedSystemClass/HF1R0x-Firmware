#include "hexabitz/ModuleInfo.h"

#include "hexabitz/ProxyModule.h"

#include <string>
#include <fstream>
#include <sstream>
#include <string.h>



/***************************************** Neighbours Info Class *****************************************/

NeighboursInfo::NeighboursInfo(void)
{
	memset(array_, 0, sizeof(array_));
}

NeighboursInfo::~NeighboursInfo(void)
{

}

BinaryBuffer NeighboursInfo::toBinaryBuffer(void) const
{
	BinaryBuffer buffer;
	// Array are stored in Row-Major Order
	for (int i = 0; i < BOS::MAX_NUM_OF_PORTS; i++) {
		if (!array_[i][ADDR_NDX])
			continue;

		buffer.append(uint8_t(i + 1));
		buffer.append(uint16_t(array_[i][ADDR_NDX]));	// 0
		buffer.append(uint16_t(array_[i][PART_NDX]));   // 1
	}
	return buffer;
}

void NeighboursInfo::fromBinaryBuffer(BinaryBuffer buffer)
{
	memset(array_, 0, sizeof(array_));
	while (buffer.getLength() >= 5) {
		uint8_t p = buffer.popui8();
		uint16_t addr = buffer.popui16();
		uint16_t part = buffer.popui16();
		// std::cout << "Addr: " << int(addr) << " " << " Part " << int(part) << std::endl;
		array_[p - 1][ADDR_NDX] = addr;
		array_[p - 1][PART_NDX] = part;
	}
}

std::string NeighboursInfo::toString(void) const
{
	std::stringstream stream;
	stream << " Neighbours Info: " << std::endl;
	for (hstd::port_t p = 1; p < BOS::MAX_NUM_OF_PORTS; p++) {
		if (!hasInfo(p))
			continue;
		stream << "        At Port: "  << p << " Module: " << BOS::toString(getPartEnumAt(p));
		stream << " ( " << getUIDAt(p) << " , " << getPortAt(p)  << " )" << std::endl;
	}


	return stream.str();
}

void NeighboursInfo::reset(void)
{
	memset(array_, 0, sizeof(array_));
}

bool NeighboursInfo::hasInfo(hstd::port_t port) const
{
	return !!(array_[port - 1][ADDR_NDX]);
}

bool NeighboursInfo::hasIDedInfo(hstd::port_t port) const
{
	return hstd::Addr_t::isValidPort(getPortAt(port)) and hstd::Addr_t::isUIDed(getUIDAt(port));
}

bool NeighboursInfo::hasUnIDedInfo(hstd::port_t port) const
{
	return hstd::Addr_t::isValidPort(getPortAt(port)) and !hstd::Addr_t::isUIDed(getUIDAt(port));
}

bool NeighboursInfo::hasAllIDedInfo(void) const
{
	for (hstd::port_t p = 1 ; p <= BOS::MAX_NUM_OF_PORTS; p++) {
		if (hasUnIDedInfo(p))
			return false;
	}
	return true;
}

void NeighboursInfo::setAddrInfoFor(hstd::port_t port, hstd::Addr_t addr)
{
	uint16_t ui16UID = addr.getUID();
	uint16_t ui16Port = addr.getPort();
	array_[port - 1][ADDR_NDX] = (ui16UID << 8) + (ui16Port & 0x00FF);
}

void NeighboursInfo::setUIDInfoFor(hstd::port_t port, hstd::uid_t uid)
{
	uint16_t ui16UID = uid;
	array_[port - 1][ADDR_NDX] = (ui16UID << 8) + (array_[port - 1][ADDR_NDX] & 0x00FF);
}

void NeighboursInfo::setPortInfoFor(hstd::port_t port, hstd::port_t neighPort)
{
	uint16_t ui16Port = neighPort;
	array_[port - 1][ADDR_NDX] = (array_[port - 1][ADDR_NDX] & 0xFF00) + (ui16Port & 0x00FF);
}

void NeighboursInfo::setPartInfoFor(hstd::port_t port, enum BOS::module_pn_e part)
{
	uint16_t ui16Part = static_cast<uint16_t>(part);
	array_[port - 1][PART_NDX] = ui16Part;
}

enum BOS::module_pn_e NeighboursInfo::getPartEnumAt(hstd::port_t port) const
{
	return static_cast<enum BOS::module_pn_e>(array_[port - 1][PART_NDX]);
}

hstd::Addr_t NeighboursInfo::getAddrAt(hstd::port_t port) const
{
	return hstd::Addr_t(getUIDAt(port), getPortAt(port));
}

hstd::uid_t NeighboursInfo::getUIDAt(hstd::port_t port) const
{
	return hstd::uid_t(highByte(array_[port - 1][ADDR_NDX]));
}

hstd::port_t NeighboursInfo::getPortAt(hstd::port_t port) const
{
	return hstd::port_t(lowByte(array_[port -1][ADDR_NDX]));
}

/*********************************************************************************************************/


struct nodeTree {

public:
	hstd::uid_t getNextUnvisited(void) const
	{
		hstd::uid_t u = 1;
		hstd::uid_t index = u;

		unsigned smallest = INF_DIST; 

		if (!isVisited(u))
			smallest = getRelDist(u);	/* Consider first element as smallest */

		for (; u <= NUM_NODES; u++) {
			if (!isVisited(u) and (getRelDist(u) < smallest)) {
				smallest = getRelDist(u);
				index = u;
			}
		}
		return index;
	}

public:
	unsigned getRelDist(hstd::uid_t uid) const
	{
		return distance_[uid - 1];
	}

	void setRelDist(hstd::uid_t uid, unsigned newDistance)
	{
		if (uid > NUM_NODES)
			return;
		distance_[uid - 1] = newDistance;
	}

	hstd::Addr_t getPrevious(hstd::uid_t uid) const
	{
		return previous_[uid - 1];
	}

	void setPrevious(hstd::uid_t uid, hstd::Addr_t node)
	{
		if (uid > NUM_NODES)
			return;
		previous_[uid - 1] = node;
	}

	bool isVisited(hstd::uid_t uid) const
	{
		return status_[uid - 1];
	}

	void markVisited(hstd::uid_t uid)
	{
		if (uid > NUM_NODES)
			return;
		status_[uid - 1] = true;
	}

	bool areAllVisited(void) const
	{
		bool temp = true;
		for (int i = 0; i < NUM_NODES; i++)
			temp &= status_[i];
		return temp;
	}

public:

	nodeTree(hstd::uid_t uid, const int nodes): own_(uid), NUM_NODES(nodes)
	{
		status_ = new bool[NUM_NODES];
		distance_ = new unsigned[NUM_NODES];
		previous_ = new hstd::Addr_t[NUM_NODES];

		memset(status_, 0, sizeof(*status_) * NUM_NODES);
		memset(distance_, INF_DIST, sizeof(*distance_) * NUM_NODES);

		distance_[uid - 1] = 0;       			// Distance from source to source
		previous_[uid - 1] = hstd::Addr_t();    // Previous node in optimal path initialization undefined
	}

	~nodeTree(void)
	{
		delete[] status_;
		delete[] distance_;
		delete[] previous_;
	}

private:
	hstd::uid_t own_;
	const int NUM_NODES;

private:
	bool *status_;
	unsigned *distance_;
	hstd::Addr_t *previous_;

public:
	static const int NODE_NODE_DIST = 1;
	static const unsigned INF_DIST = -1;
};



/******************************************* Module Info Class *******************************************/

ModulesInfo::ModulesInfo(void)
{
	memset(array_, 0, sizeof(array_));
}

ModulesInfo::~ModulesInfo(void)
{

}

BinaryBuffer ModulesInfo::toBinaryBuffer(int num) const
{
	BinaryBuffer buffer;
	if (num < 0)
		num = BOS::MAX_NUM_OF_MODULES;
	for (int i = 0; i < num; i++) {
		buffer.append(uint16_t(array_[i][PART_NUM_NDX]));

		for (int j = 0; j < BOS::MAX_NUM_OF_PORTS; j++)
			buffer.append(uint16_t(array_[i][j + 2]));
	}
	return buffer;
}

void ModulesInfo::fromBinaryBuffer(BinaryBuffer buffer)
{
	memset(array_, 0, sizeof(array_));
	
	for (int i = 0; i < BOS::MAX_NUM_OF_MODULES; i++) {
		array_[i][PART_NUM_NDX] = buffer.popui16();
		array_[i][PORT_DIR_NDX] = 0;

		for (int j = 0; j < BOS::MAX_NUM_OF_PORTS; j++)
			array_[i][j + 2] = buffer.popui16();
	}
}

bool ModulesInfo::toBinaryFile(std::string filename) const
{
	BinaryBuffer buffer;
	for (int i = 0; i < BOS::MAX_NUM_OF_MODULES; i++) {
		buffer.append(uint16_t(array_[i][PART_NUM_NDX]));
		buffer.append(uint16_t(array_[i][PORT_DIR_NDX]));

		for (int j = 0; j < BOS::MAX_NUM_OF_PORTS; j++)
			buffer.append(uint16_t(array_[i][j + 2]));
	}

	return buffer.save(filename);
}

bool ModulesInfo::fromBinaryFile(std::string filename)
{
	BinaryBuffer buffer;
	if (!buffer.restore(filename))
		return false;

	memset(array_, 0, sizeof(array_));
	for (int i = 0; i < BOS::MAX_NUM_OF_MODULES; i++) {
		array_[i][PART_NUM_NDX] = buffer.popui16();
		array_[i][PORT_DIR_NDX] = buffer.popui16();

		for (int j = 0; j < BOS::MAX_NUM_OF_PORTS; j++)
			array_[i][j + 2] = buffer.popui16();
	}

	return true;
}

std::string ModulesInfo::toString(int num) const
{
	std::stringstream stream;
	if (num < 0)
		num = BOS::MAX_NUM_OF_MODULES;

	for (int i = 0; i < num; i++) {
		stream << "Module ID: "  << (i + 1) << " [ " << BOS::toString(static_cast<enum BOS::module_pn_e>(array_[i][PART_NUM_NDX])) << " ] ";
		stream << " Port Direction: " << hstd::make_bitset(array_[i][PORT_DIR_NDX]);
		stream << " Connected To";

		for (int j = 0; j < BOS::MAX_NUM_OF_PORTS; j++) {
			hstd::uid_t uid = hstd::uid_t(array_[i][j + 2] >> 3);
			hstd::port_t port = hstd::port_t(array_[i][j + 2] & 0b111);
			if (port)
				stream << " [ Port: " << (j + 1) << " ] ( " << uid << " , " << port << " ), "; 
		}
		stream << std::endl;
	}


	return stream.str();
}

std::string ModulesInfo::toBOSFmtString(int num) const
{
	std::stringstream stream;
	if (num < 0)
		num = BOS::MAX_NUM_OF_MODULES;

	stream << "Array topology:" << std::endl << std::endl;
	// Print Header
	stream << "Module: (Port:\t";
	for (int p = 1; p <= BOS::MAX_NUM_OF_PORTS; p++) 
		stream << "\tP" << p;
	stream << ")" << std::endl << std::endl;

	for (int i = 0; i < num; i++) {
		if (!hasInfo(i + 1))
			continue;
		stream << "Module: "  << (i + 1) << " [ " << BOS::toString(static_cast<enum BOS::module_pn_e>(array_[i][PART_NUM_NDX])) << " ] ";

		for (int j = 0; j < BOS::MAX_NUM_OF_PORTS; j++) {
			hstd::uid_t uid = hstd::uid_t(array_[i][j + 2] >> 3);
			hstd::port_t port = hstd::port_t(array_[i][j + 2] & 0b111);
			if (port)
				stream << "\t" << uid << ":" << port;
			else
				stream << "\t0";
		}
		stream << std::endl;
	}

	stream << std::endl;
	return stream.str();
}

void ModulesInfo::reset(void)
{
	memset(array_, 0, sizeof(array_));
}

bool ModulesInfo::hasConnInfo(hstd::Addr_t addr) const
{
	return addr.isValid() and (array_[addr.getUID() - 1][addr.getPort() + 1] != 0);
}

bool ModulesInfo::hasConnInfo(hstd::uid_t uid, hstd::port_t port) const
{
	return hasConnInfo(hstd::Addr_t(uid, port));
}

bool ModulesInfo::hasInfo(hstd::uid_t uid) const
{
	return !!array_[uid - 1][PART_NUM_NDX];
}

bool ModulesInfo::isConnToAny(hstd::uid_t uid) const
{
	for (hstd::port_t p = 1; p <= BOS::MAX_NUM_OF_PORTS; p++) {
		if (hasConnInfo(uid, p))
			return true;
	}
	return false;
}

void ModulesInfo::addConnectionFromTo(hstd::Addr_t fromAddr, hstd::Addr_t toAddr)
{
	uint16_t ui16FromUID = fromAddr.getUID();
	uint16_t ui16FromPort = fromAddr.getPort();

	uint16_t ui16ToUID = toAddr.getUID();
	uint16_t ui16ToPort = toAddr.getPort();

	if (!hstd::Addr_t::isUIDed(ui16FromUID) or (ui16FromUID >= BOS::MAX_NUM_OF_MODULES))
		return;
	if (!hstd::Addr_t::isValidPort(ui16FromPort))
		return;
	if (!hstd::Addr_t::isUIDed(ui16ToUID) or (ui16ToUID >= BOS::MAX_NUM_OF_MODULES))
		return;
	if (!hstd::Addr_t::isValidPort(ui16ToPort))
		return;

	array_[ui16FromUID - 1][ui16FromPort + 1] = (ui16ToUID << 3 ) | (ui16ToPort & 0b111);	/* Neighbor ID | Neighbor port */
}

void ModulesInfo::clearConnectionInfoAt(hstd::Addr_t addr)
{
	if (!hstd::Addr_t::isUIDed(addr.getUID()) or (addr.getUID() >= BOS::MAX_NUM_OF_MODULES))
		return;
	if (!hstd::Addr_t::isValidPort(addr.getPort()))
		return;

	array_[addr.getUID() - 1][addr.getPort() + 1] = 0;
}

bool ModulesInfo::addConnection(hstd::Addr_t first, hstd::Addr_t second)
{
	addConnectionFromTo(first, second);
	addConnectionFromTo(second, first);
	return true;
}

void ModulesInfo::removeConnection(hstd::Addr_t first, hstd::Addr_t second)
{
	clearConnectionInfoAt(first);
	clearConnectionInfoAt(second);
}

void ModulesInfo::setPartNumOf(const ProxyModule& module)
{
	setPartNumOf(module.getUID(), module.getPartNum());
}

void ModulesInfo::setPartNumOf(hstd::Addr_t addr, enum BOS::module_pn_e part)
{
	setPartNumOf(addr.getUID(), part);
}

void ModulesInfo::setPartNumOf(hstd::uid_t uid, enum BOS::module_pn_e part)
{
	if (!hstd::Addr_t::isUIDed(uid))
		return;
	if (uid >= BOS::MAX_NUM_OF_MODULES)
		return;
	array_[uid - 1][PART_NUM_NDX] = static_cast<uint16_t>(part);
}

void ModulesInfo::setPortDir(hstd::Addr_t addr, BOS::PortDir dir)
{
	setPortDir(addr.getUID(), addr.getPort(), dir);
}

void ModulesInfo::setPortDir(hstd::uid_t uid, hstd::port_t port, BOS::PortDir dir)
{
	if (!hstd::Addr_t::isUIDed(uid))
		return;
	if (uid >= BOS::MAX_NUM_OF_MODULES)
		return;
	if (!hstd::Addr_t::isValidPort(port))
		return;

	if (dir == BOS::PortDir::REVERSED)
		array_[uid - 1][PORT_DIR_NDX] |= (0x8000 >> (port - 1));
	if (dir == BOS::PortDir::NORMAL)
		array_[uid - 1][PORT_DIR_NDX] &= (~(0x8000 >> (port - 1)));	
}

void ModulesInfo::setPortDirNormal(hstd::Addr_t addr)
{
	return setPortDir(addr, BOS::PortDir::NORMAL);
}

void ModulesInfo::setPortDirReversed(hstd::Addr_t addr)
{
	return setPortDir(addr, BOS::PortDir::REVERSED);
}

void ModulesInfo::setPortDirNormal(hstd::uid_t uid, hstd::port_t port)
{
	return setPortDir(uid, port, BOS::PortDir::NORMAL);
}

void ModulesInfo::setPortDirReversed(hstd::uid_t uid, hstd::port_t port)
{
	return setPortDir(uid, port, BOS::PortDir::REVERSED);
}

BOS::PortDir ModulesInfo::getPortDir(hstd::Addr_t addr) const
{
	return getPortDir(addr.getUID(), addr.getPort());
}

BOS::PortDir ModulesInfo::getPortDir(hstd::uid_t uid, hstd::port_t port) const
{
	if (!hstd::Addr_t::isUIDed(uid))
		return BOS::PortDir::NORMAL;
	if (uid >= BOS::MAX_NUM_OF_MODULES)
		return BOS::PortDir::NORMAL;
	if (!hstd::Addr_t::isValidPort(port))
		return BOS::PortDir::NORMAL;

	if (array_[uid - 1][PORT_DIR_NDX] & (0x8000 >> (port - 1)))
		return BOS::PortDir::REVERSED;
	return BOS::PortDir::NORMAL;
}

bool ModulesInfo::isPortDirNormal(hstd::Addr_t addr) const
{
	return getPortDir(addr) == BOS::PortDir::NORMAL;
}

bool ModulesInfo::isPortDirReversed(hstd::Addr_t addr) const
{
	return getPortDir(addr) == BOS::PortDir::REVERSED;
}

bool ModulesInfo::isPortDirNormal(hstd::uid_t uid, hstd::port_t port) const
{
	return isPortDirNormal(hstd::Addr_t(uid, port));
}

bool ModulesInfo::isPortDirReversed(hstd::uid_t uid, hstd::port_t port) const
{
	return isPortDirReversed(hstd::Addr_t(uid, port));
}

enum BOS::module_pn_e ModulesInfo::getPartEnumOf(hstd::Addr_t addr) const
{
	return getPartEnumOf(addr.getUID());
}

enum BOS::module_pn_e ModulesInfo::getPartEnumOf(hstd::uid_t uid) const
{
	if (!hstd::Addr_t::isUIDed(uid))
		return BOS::INVALID;
	if (uid >= BOS::MAX_NUM_OF_MODULES)
		return BOS::INVALID;

	return static_cast<enum BOS::module_pn_e>(array_[uid - 1][PART_NUM_NDX]);
}


hstd::uid_t ModulesInfo::getUIDOf(enum BOS::module_pn_e partNum, unsigned num) const
{
	for (hstd::uid_t i = 1; i <= BOS::MAX_NUM_OF_MODULES; i++) {
		if (partNum != static_cast<enum BOS::module_pn_e>(array_[i - 1][PART_NUM_NDX]))
			continue;
		if (!--num)
			return i;
	}
	return hstd::Addr_t::INVALID_UID;
}

hstd::Addr_t ModulesInfo::getModuleConnAt(hstd::uid_t uid, hstd::port_t port) const
{
	if (!hstd::Addr_t::isUIDed(uid))
		return hstd::Addr_t();
	if (uid >= BOS::MAX_NUM_OF_MODULES)
		return hstd::Addr_t();
	if (!hstd::Addr_t::isValidPort(port))
		return hstd::Addr_t();

	return hstd::Addr_t(hstd::uid_t(array_[uid - 1][port + 1] >> 3), hstd::port_t(array_[uid - 1][port + 1] & 0b111));
}

hstd::Addr_t ModulesInfo::getModuleConnAt(hstd::Addr_t addr) const
{
	return getModuleConnAt(addr.getUID(), addr.getPort());
}

hstd::uid_t ModulesInfo::getUIDConnAt(hstd::Addr_t addr) const
{
	return getModuleConnAt(addr).getUID();
}

hstd::port_t ModulesInfo::getPortConnAt(hstd::Addr_t addr) const
{
	return getModuleConnAt(addr).getPort();
}

hstd::uid_t ModulesInfo::getUIDConnAt(hstd::uid_t uid, hstd::port_t port) const
{
	return getModuleConnAt(uid, port).getUID();
}

hstd::port_t ModulesInfo::getPortConnAt(hstd::uid_t uid, hstd::port_t port) const
{
	return getModuleConnAt(uid, port).getPort();
}

hstd::uid_t ModulesInfo::getMaxUID(void) const
{
	hstd::uid_t lastUID = 0;
	for (hstd::uid_t i = 1; i <= BOS::MAX_NUM_OF_MODULES; i++) {
		if (hasInfo(i))
			lastUID = i;
	}

	return lastUID;
}

std::vector<hstd::Addr_t> ModulesInfo::FindRoute(hstd::Addr_t dest, hstd::Addr_t src) const
{
	const int NUM_MOD = getMaxUID();

	std::vector<hstd::Addr_t> route;
	nodeTree nodeInfo(src.getUID(), NUM_MOD);
	
	while (!nodeInfo.areAllVisited()) {

		hstd::uid_t uid = nodeInfo.getNextUnvisited();		// Source node in first case
		if (uid == dest.getUID())
			break;

		nodeInfo.markVisited(uid); 	// Mark uid as visited																					
		/* For each neighbor nID where nID is unvisited (i.e., Q[nID is false]) */
		for (hstd::port_t p = 1; p <= hstd::Addr_t::MAX_PORT; p++) {    // Check all module ports
			hstd::Addr_t curAddr(uid, p);

			if (!hasConnInfo(curAddr))	// There's a neighbor at this port p
				continue;
			
			hstd::uid_t nID = getUIDConnAt(curAddr);
			if (nodeInfo.isVisited(nID))		// nID has been visited already
				continue;												
				
			int newDist = nodeInfo.getRelDist(uid) + nodeTree::NODE_NODE_DIST;	// Add one hop
			if (newDist < nodeInfo.getRelDist(nID)) {    // A shorter path to nID has been found
				nodeInfo.setRelDist(nID, newDist);
				nodeInfo.setPrevious(nID, curAddr);
			}
		}
	}	
		
	if (nodeInfo.areAllVisited())
		return route;
	if (!nodeInfo.getPrevious(dest.getUID()).isValid())
		return route;
		
	/* Build the virtual route */
	hstd::uid_t uid = dest.getUID();
	route.push_back(hstd::Addr_t(uid, getPortConnAt(nodeInfo.getPrevious(uid))));
	while (nodeInfo.getPrevious(uid).isValid()) {  		// Construct the shortest path with a stack route
		route.push_back(nodeInfo.getPrevious(uid));		// Push the vertex onto the stack
		uid = nodeInfo.getPrevious(uid).getUID(); 	// Traverse from target to source
	}

	std::reverse(route.begin(), route.end());
	return route;			
}


std::vector<hstd::Addr_t> ModulesInfo::FindRoute(hstd::uid_t destID, hstd::uid_t srcID) const
{
	return FindRoute(hstd::Addr_t(destID), hstd::Addr_t(srcID));		
}


hstd::port_t ModulesInfo::FindSourcePort(hstd::uid_t destID, hstd::uid_t srcID) const
{
	std::vector<hstd::Addr_t> route = FindRoute(destID, srcID);
	if (route.size() <= 0)
		return 0;
	return route.at(0).getPort();
}

/*********************************************************************************************************/