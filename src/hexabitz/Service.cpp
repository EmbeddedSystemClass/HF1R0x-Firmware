#include "hexabitz/Service.h"

#include "hexabitz/ProxyModule.h"

#include <iostream>
#include <thread>
#include <chrono>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <math.h>
#include <float.h>
#include <errno.h>



Service *Service::self_ = nullptr;


Service::Service(void): serial_(nullptr)
{
	info_.reset();
	num_modules_ = 50;
}

Service::~Service(void)
{
	serial_.end();
}

Service *Service::getInstance(void)
{
	if (self_ == nullptr)
		self_ = new Service();
	if (self_ == nullptr)	// TODO: Handle this. Throw Exception
		return self_;
	return self_;
}

void Service::osDelay(int milliseconds)
{
	std::this_thread::sleep_for(std::chrono::milliseconds(milliseconds));
}	

int Service::init(const char *pathname)
{
	if (!serial_.open(pathname))
		return -EIO;

	std::cout << "Starting Serial Port" << std::endl;
	serial_.begin(DEFAULT_BAUDRATE);
	return 0;
}

void Service::setOwn(std::shared_ptr<ProxyModule> module)
{
	owner_ = module;
}

std::shared_ptr<ProxyModule> Service::getOwn(void)
{
	return owner_;
}

int Service::send(const hstd::Message& msg)
{
	int ret = 0;
	std::vector<hstd::Frame> list = hstd::buildFramesFromMessage(msg);

	for (int i = 0; i < list.size(); i++) {
		hstd::Frame& f = list[i];
		std::cout << "Sending (" << i << "th): " << f << std::endl;
		if ((ret = send(f))) return ret;
	}

	return 0;
}

int Service::send(const hstd::Frame& f)
{
	if (!f.isValid())
		return -EINVAL;

	BinaryBuffer buffer = f.toBuffer();
	for (int i = 0; i < buffer.getLength(); i++) 
		serial_.write(buffer[i]);

	return 0;
}


int Service::receive(hstd::Message& msg, long timeout)
{
	std::vector<hstd::Frame> list;
	while (1) {
		hstd::Frame f;
		if (!receive(f, timeout))
			return false;

		std::cout << "Received: " << f << std::endl;
		list.push_back(f);

		if (hstd::buildMessageFromFrames(list, msg))
			return 0;
	}
	return -EAGAIN;
}

int Service::receive(hstd::Frame& f, long timeout)
{
	BinaryBuffer buffer;

	while (1) {
		if (!serial_.available())
			continue;

		uint8_t c = serial_.read();
		f.param.reset();
		buffer.append(c);
		if (f.fromBuffer(buffer))
			return 0;
	}
	return -EAGAIN;
}


std::vector<hstd::Addr_t> Service::FindRoute(hstd::Addr_t dest, hstd::Addr_t src)
{
	uint8_t Q[BOS::MAX_NUM_OF_MODULES] = { 0 };		// All nodes initially in Q (unvisited nodes)

	auto minUnvisitedUID = [this](uint8_t *arr, uint8_t *Q) -> uint8_t {
		uint8_t index = 0, smallest = 0xFF; 

		if (!Q[0]) smallest = arr[0];	/* Consider first element as smallest */

		for (int i = 0; i < this->num_modules_; i++) {
			if ((arr[i] < smallest) && !Q[i]) {
				smallest = arr[i];
				index = i;
			}
		}
		return index;
	};

	auto isQNotEmpty = [this](uint8_t *Q) {
		char temp = 1;
		for (int i = 0; i < this->num_modules_; i++)
			temp &= Q[i];
		return temp;
	};

	std::vector<hstd::Addr_t> route;
	hstd::Addr_t routePrev[BOS::MAX_NUM_OF_MODULES];
	uint8_t routeDist[BOS::MAX_NUM_OF_MODULES] = { 0 };

	hstd::uid_t sourceID = src.getUID();
	hstd::uid_t destID = dest.getUID();
	
	/* Initialization */
	// memset(Service::route, 0, sizeof(Service::route));
	memset(routeDist, 0xFF, sizeof(routeDist));
	// memset(routePrev, 0, sizeof(routePrev));

	routeDist[sourceID - 1] = 0;       				// Distance from source to source
	routePrev[sourceID - 1] = hstd::Addr_t();       // Previous node in optimal path initialization undefined
	
	/* Algorithm */
	uint8_t currentID = 0;
	while (!isQNotEmpty(Q)) {

		currentID = minUnvisitedUID(routeDist, Q) + 1;		// Source node in first case
		if (currentID == destID)
			goto finishedRoute;

		Q[currentID - 1] = 1;								// Remove u from Q 																					
		/* For each neighbor v where v is still in Q. */
		for (uint8_t p = 1; p <= 6; p++) {     				// Check all module ports
			if (!info_.hasConnInfo(currentID, p))		// There's a neighbor v at this port n
				continue;
			
			hstd::uid_t nID = info_.getUIDConnAt(hstd::Addr_t(currentID, p));
			if (Q[nID - 1])		// v is not in Q
				continue;												
				
			uint8_t newDist = routeDist[currentID - 1] + 1;		// Add one hop
			if (newDist < routeDist[nID - 1]) {     	// A shorter path to v has been found
				routeDist[nID - 1] = newDist; 
				routePrev[nID - 1] = hstd::Addr_t(currentID, p); 
			}
		}
	}	
		
finishedRoute:
		
	/* Build the virtual route */
	// currentID = routePrev[currentID - 1].getUID();
	route.push_back(hstd::Addr_t(routePrev[currentID - 1]));
	while (routePrev[currentID - 1].isValid()) {  		// Construct the shortest path with a stack route
		route.push_back(routePrev[currentID - 1]);		// Push the vertex onto the stack
		currentID = routePrev[currentID - 1].getUID(); 	// Traverse from target to source
	}

	std::reverse(route.begin(), route.end());
	return route;			
}


uint8_t Service::FindRoute(uint8_t src, uint8_t dest)
{
	std::vector<hstd::Addr_t> route = FindRoute(hstd::Addr_t(dest, 1), hstd::Addr_t(src, 1));
	if (route.size() <= 0)
		return 0;
	return route.at(0).getPort();
}

uint8_t Service::FindSourcePort(uint8_t srcID, uint8_t destID)
{
	std::vector<hstd::Addr_t> route = FindRoute(hstd::Addr_t(destID, 1), hstd::Addr_t(srcID, 1));
	if (route.size() <= 0)
		return 0;
	return route.at(0).getPort();
}


int Service::ping(uint8_t destID)
{
	int ret = 0;
	hstd::Message msg = hstd::make_message(destID, CODE_ping);

	if ((ret = send(msg)))
		return ret;
	if ((ret = receive(msg)))
		return ret;
	return 0;
}


int Service::assignIDToNeigh(hstd::uid_t id, hstd::port_t portNum)
{
	int ret = 0;
	hstd::Message msg = hstd::make_message_meighbour(portNum, CODE_module_id);

	msg.getParams().append(uint8_t(0));	/* Change own ID */
	msg.getParams().append(uint8_t(id));
	msg.getParams().append(uint8_t(0));

	if ((ret = send(msg)))
		return ret;
	// No response to this message
	return 0;
}

int Service::assignIDToAdjacent(uint8_t destID, uint8_t portNum, uint8_t newID)
{
	int ret = 0;
	hstd::Message msg = hstd::make_message(destID, CODE_module_id);
	msg.getParams().append(uint8_t(1));	/* Change own ID */
	msg.getParams().append(newID);
	msg.getParams().append(portNum);

	return send(msg);	// No response to this message
}

int Service::sayHiToNeighbour(uint8_t portOut, enum BOS::module_pn_e& part, hstd::Addr_t& neigh)
{
	int ret = 0;
	/* Port, Source = 0 (myID), Destination = 0 (adjacent neighbor), message code, number of parameters */
	hstd::Message msg = hstd::make_message_meighbour(portOut, CODE_hi);

	msg.getParams().append(static_cast<uint16_t>(getOwn()->getPartNum()));
	msg.getParams().append(portOut);
	
	if ((ret = send(msg)))
		return ret;
	if ((ret = receive(msg)))
		return ret;
	if (msg.getCode() != CODE_hi_response)
		return -EAGAIN;

	/* Neighbor PN */
	part = static_cast<enum BOS::module_pn_e>(msg.getParams().popui16());
	/* Neighbor ID + Neighbor own port */
	neigh.setUID(msg.getSource().getUID());
	neigh.setPort(msg.getParams().popui8());
	return 0;
}

NeighboursInfo Service::ExploreNeighbors(uint8_t ignore)
{
	int ret = 0;
	NeighboursInfo info;
	const int NUM_PORTS = getOwn()->getNumOfPorts();

	/* Send Hi messages to adjacent neighbors */
	for (int port = 1; port <= NUM_PORTS; port++) {
		hstd::Addr_t neigh;
		enum BOS::module_pn_e part;

		if (port == ignore)
			continue;
		if ((ret = sayHiToNeighbour(port, part, neigh)))
			break;

		info.setAddrInfoFor(port, neigh);
		info.setPartInfoFor(port, part);
	}
	return info;
}

int Service::ExploreAdjacentOf(hstd::Addr_t addr, NeighboursInfo& info)
{
	int ret = 0;
	hstd::Message msg = hstd::make_message(addr, CODE_explore_adj);
	if ((ret = send(msg)))
		return ret;
	if ((ret = receive(msg)))
		return ret;

	if (msg.getCode() != CODE_explore_adj_response)
		return -EAGAIN;

	info.reset();
	info.fromBinaryBuffer(msg.getParams());
	return ret;
}

void Service::changePortDir(int port, enum BOS::PortDir dir)
{
	// We don't have anything to do in hardware
	// We can change the direction of the port
	if (dir == BOS::PortDir::REVERSED)
		return;
	return;
}


int Service::syncTopologyTo(hstd::uid_t destID)
{
	hstd::Message msg = hstd::make_message(hstd::Addr_t(destID), CODE_topology);
	BinaryBuffer buffer = info_.toBinaryBuffer(num_modules_);
	msg.getParams().append(buffer);

	return send(msg);
}

int Service::synPortDir(hstd::uid_t dest)
{
	int ret = 0;
	hstd::Message msg = hstd::make_message(dest, CODE_port_dir);

	for (hstd::port_t p = 1 ; p <= BOS::MAX_NUM_OF_PORTS; p++) {

		hstd::Addr_t addr = hstd::Addr_t(dest, p);
		hstd::Addr_t nAddr = info_.getModuleConnAt(addr);

		if (!info_.hasConnInfo(addr) or info_.isPortDirReversed(nAddr))	{
			/* If empty port leave BOS::PortDir::NORMAL */
			msg.getParams().append(uint8_t(BOS::PortDir::NORMAL));
			info_.setPortDirNormal(addr);
		} else {
			msg.getParams().append(uint8_t(BOS::PortDir::REVERSED));
			info_.setPortDirReversed(addr);			
		}
	}
	
	/* Step 5c - Check if an inport is BOS::PortDir::REVERSED */
	/* Find out the inport to this module from master */
	std::vector<hstd::Addr_t> route = FindRoute(hstd::Addr_t(1), hstd::Addr_t(dest));
	hstd::uid_t justNextMod = route[1].getUID();				/* previous module = route[Number of hops - 1] */
	hstd::port_t portOut = FindRoute(dest, justNextMod);

	/* Is the inport BOS::PortDir::REVERSED? */
	if ((justNextMod == dest) or (msg.getParams()[portOut - 1] == uint8_t(BOS::PortDir::REVERSED)) )
		msg.getParams().append(uint8_t(BOS::PortDir::REVERSED));		/* Make sure the inport is BOS::PortDir::REVERSED */
	else
		msg.getParams().append(uint8_t(0));

	if ((ret = send(msg)))
		return ret;

	osDelay(10);
	return 0;
}

int Service::broadcastToSave(void)
{
	int ret = 0;
	hstd::Message msg = hstd::make_broadcast(CODE_exp_eeprom);
	if ((ret = send(msg)))
		return ret;
	return 0;
}

int Service::reverseAllButInPort(hstd::uid_t destID)
{
	int ret = 0;
	hstd::Message msg = hstd::make_message(hstd::Addr_t(destID), CODE_port_dir);

	for (uint8_t p = 1 ; p <= BOS::MAX_NUM_OF_PORTS; p++)
		msg.getParams().append(uint8_t(BOS::PortDir::REVERSED));
	msg.getParams().append(uint8_t(BOS::PortDir::NORMAL)); /* Make sure the inport is not BOS::PortDir::REVERSED */

	if ((ret = send(msg)))
		return ret;
	return 0;
}



int Service::Explore(void)
{
	int result = 0;
	hstd::port_t PcPort = 1;
	NeighboursInfo neighInfo;

	std::shared_ptr<ProxyModule> master = getOwn();
	if (master == nullptr)
		return -EINVAL;
	
	master->setUID(hstd::Addr_t::MASTER_UID);
	info_.setPartNumOf(*master);

	hstd::uid_t myID = master->getUID();
	const int NUM_OF_PORTS = getOwn()->getNumOfPorts();

	hstd::uid_t lastID = 0;
	hstd::uid_t currentID = myID;
	
	/* >>> Step 1 - Reverse master ports and explore adjacent neighbors */
	for (hstd::port_t port = 1; port <= NUM_OF_PORTS; port++) {
		if (port != PcPort) changePortDir(port, BOS::PortDir::REVERSED);
	}
	neighInfo = ExploreNeighbors(PcPort);

	
	/* >>> Step 2 - Assign IDs to new modules & update the topology array */
	for (hstd::port_t port = 1; port <= NUM_OF_PORTS; port++) {
		if (!neighInfo.hasInfo(port))
			continue;

		/* Step 2a - Assign IDs to new modules */
		if (assignIDToNeigh(++currentID, port))
			continue;

		neighInfo.setUIDInfoFor(port, currentID);

		hstd::Addr_t ownAddr = hstd::Addr_t(myID, port);
		hstd::Addr_t neighAddr = neighInfo.getAddrAt(port);
		enum BOS::module_pn_e neighPart = neighInfo.getPartEnumAt(port);

		/* Step 2b - Update master topology array */	
		info_.addConnection(ownAddr, neighAddr);
		info_.setPartNumOf(neighAddr, neighPart);

		num_modules_ = currentID;
	}
	
	/* Step 2c - Ask neighbors to update their topology array */
	for (hstd::uid_t i = 2; i <= currentID; i++) {
		syncTopologyTo(i);
		osDelay(60);
	}
	
	
	/* >>> Step 3 - Ask each new module to explore and repeat */
	
	while (lastID != currentID) {
		/* Update lastID */
		lastID = currentID;
		
		/* Scan all discovered modules */
		for (hstd::uid_t i = 2 ; i <= currentID; i++) {
			/* Step 3a - Ask the module to reverse ports */
			reverseAllButInPort(i);
			osDelay(10);
			
			/* Step 3b - Ask the module to explore adjacent neighbors */
			NeighboursInfo adjInfo;
			ExploreAdjacentOf(hstd::Addr_t(i), adjInfo);	
		
			for (hstd::port_t p = 1; p <= BOS::MAX_NUM_OF_PORTS; p++) {
				if (!adjInfo.hasInfo(p))
					continue;

				/* Step 3c - Assign IDs to new modules */
				assignIDToAdjacent(i, p, ++currentID);
				adjInfo.setUIDInfoFor(p, currentID);

				hstd::Addr_t ithAddr = hstd::Addr_t(i, p);
				hstd::Addr_t newAddr = adjInfo.getAddrAt(p);
				enum BOS::module_pn_e neighPart = adjInfo.getPartEnumAt(p);

				/* Step 3d - Update master topology array */
				if (newAddr.getUID() == hstd::Addr_t::MASTER_UID)
					continue;
				info_.addConnection(ithAddr, newAddr);
				info_.setPartNumOf(newAddr, neighPart);

				num_modules_ = currentID;
				osDelay(10);
			}

			/* Step 3e - Ask all discovered modules to update their topology array */
			syncTopologyTo(i);
			osDelay(60);
		}	
	}

	
	/* >>> Step 4 - Make sure all connected modules have been discovered */
	
	neighInfo = ExploreNeighbors(PcPort);
	/* Check for any unIDed neighbors */
	if (neighInfo.hasAllIDedInfo())
		result = -EINVAL;

	/* Ask other modules for any unIDed neighbors */
	for (hstd::uid_t i = 2; i <= currentID; i++) {
		NeighboursInfo adjInfo;
		ExploreAdjacentOf(hstd::Addr_t(i), adjInfo);

		if (adjInfo.hasAllIDedInfo())
			result = -EAGAIN;			
	}
	
	
	/* >>> Step 5 - If no unIDed modules found, generate and distribute port directions */
	if (result)
		goto END;

	/* Step 5a - Virtually reset the state of master ports to BOS::PortDir::NORMAL */
	for (hstd::port_t p = 1; p <= NUM_OF_PORTS; p++)
		info_.setPortDirNormal(master->getUID(), p);
	
	/* Step 5b - Update other modules ports starting from the last one */
	for (hstd::uid_t i = currentID; i >= 2; i--) {
		if (synPortDir(i))
			continue;		
	}			

	/* Step 5e - Update master ports > all BOS::PortDir::NORMAL */
	
			
	/* >>> Step 6 - Test new port directions by pinging all modules */
	
	if (result)
		goto END; 

	Service::osDelay(100);
	// BOS.response = BOS_RESPONSE_MSG;		// Enable response for pings
	for (hstd::uid_t i = 2; i <= num_modules_; i++) {
		if (ping(i)) {
			result = -EINVAL;
			break;
		}
	}
	
	/* >>> Step 7 - Save all (topology and port directions) in RO/EEPROM */
	if (result)
		goto END;
	/* Save data in the master */
	// SaveToRO();
	// SaveEEportsDir();
	osDelay(100);
	/* Ask other modules to save their data too */
	broadcastToSave();
	

END:
	return result;
}
