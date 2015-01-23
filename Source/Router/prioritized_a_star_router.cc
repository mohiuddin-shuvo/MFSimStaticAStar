/*
 * prioritized_a_star_router.cc
 *
 *  Created on: Dec 2, 2014
 *  Authors: Mohiuddin Abdul Qader, Abhinand Menon
 */


#include "../../Headers/Router/prioritized_a_star_router.h"
#include <algorithm>
#include <cstdlib>
#include <ctime>
#include <stack>
#include <set>



///////////////////////////////////////////////////////////////////////////////////
// Constructors
///////////////////////////////////////////////////////////////////////////////////
PrioritizedAStarRouter::PrioritizedAStarRouter()
{
	board = NULL;
	claim(false, "Invalid constructor used for Router variant.  Must use form that accepts DmfbArch.\n");
}
PrioritizedAStarRouter::PrioritizedAStarRouter(DmfbArch *dmfbArch)
{
	board = NULL;
	arch = dmfbArch;
}

void PrioritizedAStarRouter::printBlockages()
{
	cout << "---------------------------------------" << endl;
	for(int i = 0; i < arch->getNumCellsY(); ++i )
	{
		cout << i%10 << " ";
		for(int k = 0; k < arch->getNumCellsX(); ++k)
		{
			if(board->at(k)->at(i)->block)
				cout << " B ";
			else
				cout << " - ";
		}
		cout << endl;

	}
	cout << endl << "---------------------------------------------";
}

///////////////////////////////////////////////////////////////////////////////////
// Deconstructor
///////////////////////////////////////////////////////////////////////////////////
PrioritizedAStarRouter::~PrioritizedAStarRouter()
{
	//cerr << "Destructor" << endl;
	if (board)
	{
		while (!board->empty())
		{
			vector<PrStarCell*> *v = board->back();
			board->pop_back();
			while (!v->empty())
			{
				PrStarCell *c = v->back();
				v->pop_back();
				delete c;
			}
			delete v;
		}
		delete board;
	}
}

///////////////////////////////////////////////////////////////////////////////////
// Initializes the board full of PrStarCells instead of Soukup cells for Lee's
///////////////////////////////////////////////////////////////////////////////////
void PrioritizedAStarRouter::routerSpecificInits()
{
	// Create a 2D-array of Lee cells
	board = new vector<vector<PrStarCell *> *>();
	for (int x = 0; x < arch->getNumCellsX(); x++)
	{
		vector<PrStarCell *> *col = new vector<PrStarCell *>();
		for (int y = 0; y < arch->getNumCellsY(); y++)
		{
			PrStarCell *c = new PrStarCell();
			c->x = x;
			c->y = y;
			c->block = false;
			c->isUsed = false;
			c->came_from = NULL;
			c->score = 0;
			c->noOfStalls = 0;
			col->push_back(c);
		}
		board->push_back(col);
	}

	if (type == PR_A_STAR_TYPE_1R_R || type == PR_A_STAR_TYPE_2R_R || type == PR_A_STAR_TYPE_3R_R) {
		// If droplets are to be routed in random order, initialize seed for PRNG
		srand(time(NULL));
	};
}



///////////////////////////////////////////////////////////////////////////////////
void PrioritizedAStarRouter::computeIndivSupProbRoutes(vector<vector<RoutePoint *> *> *subRoutes, vector<Droplet *> *subDrops, map<Droplet *, vector<RoutePoint *> *> *routes)
{
	// Get the nodes that need to be routed this time step
	vector<AssayNode *> routableThisTS;
	for (unsigned i = 0; i < thisTS->size(); i++)
		if (thisTS->at(i)->GetType() != DISPENSE && thisTS->at(i)->GetStartTS() == startingTS)
			routableThisTS.push_back(thisTS->at(i));

	// Gather the source and destination cells of each droplet to be routed this time step
	// For now, assume a non-io source is at the location of its last route point; non-io destination is bottom-left
	map<Droplet *, PrStarCell *> *sourceCells = new map<Droplet *, PrStarCell *>();
	map<Droplet *, PrStarCell *> *targetCells = new map<Droplet *, PrStarCell *>();
	set<ReconfigModule *> storageModsWithMultDrops;
	vector<Droplet *> *routingThisTS = new vector<Droplet *>();
	vector<PrStarCell*> blockCells;
	PrStarCell *s = NULL;
	PrStarCell *t = NULL;
	int score_temp = 0;
	for (unsigned i = 0; i < routableThisTS.size(); i++)
	{
		AssayNode *n = routableThisTS.at(i);
		for (unsigned p = 0; p < n->GetParents().size(); p++)
		{
			routeCycle = cycle;// DTG added to compact
			AssayNode *par = n->GetParents().at(p);
			Droplet *pd = par->GetDroplets().back();
			routingThisTS->push_back(pd);
			par->droplets.pop_back();
			n->addDroplet(pd);

			if (n->GetReconfigMod())
				n->GetReconfigMod()->incNumDrops();

			// First get sources
			s = NULL;
			if (par->GetType() == DISPENSE)
			{
				if (par->GetIoPort()->getSide() == NORTH)
					s = board->at(par->GetIoPort()->getPosXY())->at(0);
				else if (par->GetIoPort()->getSide() == SOUTH)
					s = board->at(par->GetIoPort()->getPosXY())->at(arch->getNumCellsY()-1);
				else if (par->GetIoPort()->getSide() == EAST)
					s = board->at(arch->getNumCellsX()-1)->at(par->GetIoPort()->getPosXY());
				else if (par->GetIoPort()->getSide() == WEST)
					s = board->at(0)->at(par->GetIoPort()->getPosXY());
			}
			else
				s = board->at((*routes)[pd]->back()->x)->at((*routes)[pd]->back()->y); // last route point
			sourceCells->insert(pair<Droplet *, PrStarCell *>(pd, s));

			// Now get targets
			t = NULL;
			if (n->GetType() == OUTPUT)
			{
				if (n->GetIoPort()->getSide() == NORTH)
					t = board->at(n->GetIoPort()->getPosXY())->at(0);
				else if (n->GetIoPort()->getSide() == SOUTH)
					t = board->at(n->GetIoPort()->getPosXY())->at(arch->getNumCellsY()-1);
				else if (n->GetIoPort()->getSide() == EAST)
					t = board->at(arch->getNumCellsX()-1)->at(n->GetIoPort()->getPosXY());
				else if (n->GetIoPort()->getSide() == WEST)
					t = board->at(0)->at(n->GetIoPort()->getPosXY());
			}
			else
			{
				if (n->GetType() == STORAGE && n->GetReconfigMod()->getNumDrops() > 1)
				{
					t = board->at(n->GetReconfigMod()->getLX())->at(n->GetReconfigMod()->getTY()); // Top-Left if second Storage drop
					storageModsWithMultDrops.insert(n->GetReconfigMod());
				}
				else
					t = board->at(n->GetReconfigMod()->getLX())->at(n->GetReconfigMod()->getBY()); // Bottom-Left, else// DTG, this will need to be adjusted for storage etc., when/if more than one destination in a module
			}
			targetCells->insert(pair<Droplet *, PrStarCell *>(pd, t));
		}
	}


		// Check the droplets in "storage modules with multiple droplets"
		// Ensures that droplets if one of the droplet's last routing points
		// is the other's target, that they switch targets so the droplet(s)
		// already at a valid destination does not have to move.
		set<ReconfigModule *>::iterator modIt = storageModsWithMultDrops.begin();
		for (; modIt != storageModsWithMultDrops.end(); modIt++)
		{
			ReconfigModule *rm = *modIt;

			// Now we know what module has 2+ droplets, need to search possible
			// nodes to find droplets
			vector<Droplet *> moduleDroplets;
			for (unsigned i = 0; i < routableThisTS.size(); i++)
				if (routableThisTS.at(i)->GetReconfigMod() == rm)
					moduleDroplets.push_back(routableThisTS.at(i)->GetDroplets().front());

			// Now, examine these droplets - brute force fine since number is low
			for (unsigned i = 0; i < moduleDroplets.size()/2+1; i++)
			{
				for (unsigned j = i+1; j < moduleDroplets.size(); j++)
				{
					Droplet *d1 = moduleDroplets.at(i);
					Droplet *d2 = moduleDroplets.at(j);
					RoutePoint *rp1 = (*routes)[d1]->back(); // Last rp for d1
					RoutePoint *rp2 = (*routes)[d2]->back(); // Last rp for d2
					PrStarCell *t1 = targetCells->at(d1);
					PrStarCell *t2 = targetCells->at(d2);

					if ((rp1->x == t2->x && rp1->y == t2->y) || (rp2->x == t1->x && rp2->y == t1->y))
					{
						//cout << "Switched!"  << endl;
						targetCells->erase(d1);
						targetCells->erase(d2);
						targetCells->insert(pair<Droplet *, PrStarCell *>(d1, t2));
						targetCells->insert(pair<Droplet *, PrStarCell *>(d2, t1));
					}

				}
			}
		}


    // Sort droplets based on selected priority scheme
    reorderDropletsByPriority(sourceCells, targetCells, routingThisTS);


    // Now route droplets according to the
	for(unsigned i = 0; i < routingThisTS->size(); ++i)
	{
		vector<PrStarCell*> *open_set = new vector<PrStarCell*>();
		vector<PrStarCell* > *closed_set = new vector<PrStarCell*>();
		vector<RoutePoint *> *sr = new vector<RoutePoint *>();
		vector<PrStarCell*> neighbors;// = new vector<PrStarCell*>();
		score_temp = 0;
		routeCycle = cycle;


		// Add current droplet to subDrops
		Droplet *d = routingThisTS->at(i);
		subDrops->push_back(d);

		//Loop used to block off cells that have a droplet in them

		s = sourceCells->at(d);
		s->score = 0;
		t = targetCells->at(d);

		if ((*routes)[d]->empty())
		{
			RoutePoint *rp = new RoutePoint();
			rp->cycle = routeCycle;
			//routeCycle++;
			rp->dStatus = DROP_NORMAL;
			rp->x = s->x;
			rp->y = s->y;
			sr->push_back(rp);
		}

		//Reset board for new route
		for (int x = 0; x < arch->getNumCellsX(); x++)
		{
			for (int y = 0; y < arch->getNumCellsY(); y++)
			{
				PrStarCell *c = board->at(x)->at(y);
				c->came_from = NULL;
				c->score = 0;
				c->block = false;
				c->noOfStalls = 0 ;
			}
		}

		//Blockages for modules
		for (unsigned i = 0; i < thisTS->size(); i++)
		{
			AssayNode *n = thisTS->at(i);
			if (!(n->GetType() == DISPENSE || n->GetType() == OUTPUT))
			{	// Block all new reconfigurable areas from being used for routing
				ReconfigModule *rm = n->GetReconfigMod();
				if (n->startTimeStep < startingTS && n->endTimeStep > startingTS)
				{
						for (int x = rm->getLX()-1; x <= rm->getRX()+1; x++)
							for (int y = rm->getTY()-1; y <= rm->getBY()+1; y++)
								if (x >= 0 && y >= 0 && x < arch->getNumCellsX() && y < arch->getNumCellsY())
									board->at(x)->at(y)->block = true;
					}
			}
		}


		//Blockages for other droplets
		for (unsigned j = 0; j < routingThisTS->size(); j++)
				{
					Droplet *d2 = routingThisTS->at(j);
					if (d != d2)
					{
						PrStarCell *c = sourceCells->at(d2);
						for (int x = c->x-1; x <= c->x+1; x++)
							for (int y = c->y-1; y <= c->y+1; y++)
								if (x >= 0 && y >= 0 && x < arch->getNumCellsX() && y < arch->getNumCellsY()) // On board?
											if (!(*routes)[d2]->empty()) // Don't mark as blockage for dispense droplets; they wait in reservoir
											{
												board->at(x)->at(y)->block = true;
											}

										c = targetCells->at(d2);
										for (int x = c->x-1; x <= c->x+1; x++)
											for (int y = c->y-1; y <= c->y+1; y++)
												if (x >= 0 && y >= 0 && x < arch->getNumCellsX() && y < arch->getNumCellsY())
												{// On board?
															board->at(x)->at(y)->block = true;
									}

					}
					}




		PrStarCell * currCell = s;
		open_set->push_back(s);
		//while loop to go through A*
		int ts = -1;
		while(!open_set->empty())
		{
			currCell = score_min(open_set, t);

			if(currCell == t)
			{
				sr = find_path(s,t);

			}

			ts++;
			int cx = currCell->x; // Current x,y
			int cy = currCell->y;
			currCell->isUsed = true;
			int currCycle = 0;
			PrStarCell *next = currCell;
			vector<RoutePoint *> *sr = new vector<RoutePoint *>();
			while(next->x != s->x || next->y != s->y)
			{
				currCycle++;
				next = next->came_from;
			}
			//grab the neighbor cells to find best possible
			int shouldStall = 0;
			if (cx+1 >= 0 && cx+1 < arch->getNumCellsX() && cy >= 0 && cy < arch->getNumCellsY())
			{// Right neighbor
				bool flag = true;

                //Check whether it conflicts with higher priority droplets
				for (unsigned j = 0; j < routingThisTS->size(); j++)
				{
					Droplet *d2 = routingThisTS->at(j);
					if (d == d2)
						break;
					if(currCycle<=subRoutes->at(j)->size()-1 &&  cx+1 == subRoutes->at(j)->at(currCycle)->x && cy == subRoutes->at(j)->at(currCycle)->y)
					{
						flag = false;
						break;
					}

					if(currCycle-1<=subRoutes->at(j)->size()-1)
					{
						if(cx+1 == subRoutes->at(j)->at(currCycle-1)->x && cy == subRoutes->at(j)->at(currCycle-1)->y)
						{
							flag = false;
							break;
						}
					}
					/*if(subRoutes->at(j)->size()-1<currCycle)
					{
						if(cx+1 == subRoutes->at(j)->at(subRoutes->at(j)->size()-1)->x && cy == subRoutes->at(j)->at(subRoutes->at(j)->size()-1)->y)
						{
							flag = false;
							break;
						}

					}*/
				}




				if(flag)
					neighbors.push_back(board->at(cx+1)->at(cy));
				else
					shouldStall++;

			}

			if (cx-1 >= 0 && cx-1 < arch->getNumCellsX() && cy >= 0 && cy < arch->getNumCellsY()) // Left neighbor
			{
                //Check whether it conflicts with higher priority droplets
				bool flag = true;
				for (unsigned j = 0; j < routingThisTS->size(); j++)
				{
					Droplet *d2 = routingThisTS->at(j);
					if (d == d2)
						break;
					if(currCycle<=subRoutes->at(j)->size()-1 &&  cx-1 == subRoutes->at(j)->at(currCycle)->x && cy == subRoutes->at(j)->at(currCycle)->y)
					{
						flag = false;
						break;
					}

					if(currCycle-1<=subRoutes->at(j)->size()-1)
					{
						if(cx-1 == subRoutes->at(j)->at(currCycle-1)->x && cy == subRoutes->at(j)->at(currCycle-1)->y)
						{
							flag = false;
							break;
						}
					}
					/*
					if(subRoutes->at(j)->size()-1<currCycle)
					{
						if(cx-1 == subRoutes->at(j)->at(subRoutes->at(j)->size()-1)->x && cy == subRoutes->at(j)->at(subRoutes->at(j)->size()-1)->y)
						{
							flag = false;
							break;
						}

					}*/
				}
				if(flag)
					neighbors.push_back(board->at(cx-1)->at(cy));
				else
					shouldStall++;

			}
			if (cx >= 0 && cx < arch->getNumCellsX() && cy+1 >= 0 && cy+1 < arch->getNumCellsY()) // Bottom neighbor
			{
				bool flag = true;
                //Check whether it conflicts with higher priority droplets
				for (unsigned j = 0; j < routingThisTS->size(); j++)
				{
					Droplet *d2 = routingThisTS->at(j);
					if (d == d2)
						break;
					if(currCycle<=subRoutes->at(j)->size()-1 &&  cx == subRoutes->at(j)->at(currCycle)->x && cy+1 == subRoutes->at(j)->at(currCycle)->y)
					{
						flag = false;
						break;
					}

					if(currCycle-1<=subRoutes->at(j)->size()-1)
					{
						if(cx == subRoutes->at(j)->at(currCycle-1)->x && cy+1 == subRoutes->at(j)->at(currCycle-1)->y)
						{
							flag = false;
							break;
						}
					}
					/*
					if(subRoutes->at(j)->size()-1<currCycle)
					{
						if(cx == subRoutes->at(j)->at(subRoutes->at(j)->size()-1)->x && cy+1 == subRoutes->at(j)->at(subRoutes->at(j)->size()-1)->y)
						{
							flag = false;
							break;
						}

					}*/
				}
				if(flag)
					neighbors.push_back(board->at(cx)->at(cy+1));
				else
					shouldStall++;


			}
			if (cx >= 0 && cx < arch->getNumCellsX() && cy-1 >= 0 && cy-1 < arch->getNumCellsY()) // Top neighbor
			{
                //Check whether it conflicts with higher priority droplets
				bool flag = true;
				for (unsigned j = 0; j < routingThisTS->size(); j++)
				{
					Droplet *d2 = routingThisTS->at(j);
					if (d == d2)
						break;
					if(currCycle<=subRoutes->at(j)->size()-1 &&  cx == subRoutes->at(j)->at(currCycle)->x && cy-1 == subRoutes->at(j)->at(currCycle)->y)
					{
						flag = false;
						break;
					}

					if(currCycle-1<=subRoutes->at(j)->size()-1)
					{
						if(cx == subRoutes->at(j)->at(currCycle-1)->x && cy-1 == subRoutes->at(j)->at(currCycle-1)->y)
						{
							flag = false;
							break;
						}
					}
					/*
					if(subRoutes->at(j)->size()-1<currCycle)
					{
						if(cx == subRoutes->at(j)->at(subRoutes->at(j)->size()-1)->x && cy-1 == subRoutes->at(j)->at(subRoutes->at(j)->size()-1)->y)
						{
							flag = false;
							break;
						}

					}*/
				}
				if(flag)
					neighbors.push_back(board->at(cx)->at(cy-1));
				else
					shouldStall++;

			}
			//if(shouldStall==4)
            //Add Stall as a possible transition
            neighbors.push_back(board->at(cx)->at(cy));

			for(unsigned k = 0; k < neighbors.size(); ++k)
			{
				neighbors[k]->score = currCell->score+1;
				if(neighbors[k]->block)
				{
					neighbors[k]->score = 100000;
				}
				score_temp = neighbors[k]->score + 1;
				if ((type == PR_A_STAR_TYPE_2S_R ||type == PR_A_STAR_TYPE_2L_R ||type == PR_A_STAR_TYPE_2R_R ||type == PR_A_STAR_TYPE_2X_R ||type == PR_A_STAR_TYPE_2Y_R ||type == PR_A_STAR_TYPE_2B_R)
						  && !neighbors[k]->isUsed )
					neighbors[k]->score += 10;
				else if ((type == PR_A_STAR_TYPE_3S_R ||type == PR_A_STAR_TYPE_3L_R ||type == PR_A_STAR_TYPE_3R_R ||type == PR_A_STAR_TYPE_3X_R ||type == PR_A_STAR_TYPE_3Y_R ||type == PR_A_STAR_TYPE_3B_R)
						  && neighbors[k]->isUsed )
					neighbors[k]->score += 10;

				if(is_in_evaluated_set(closed_set , neighbors[k]))
				{
				}
				else if(!is_in_evaluated_set(open_set,neighbors[k]) || score_temp < neighbors[k]->score)// && (!find(blockCells.begin(),blockCells.end(),neighbors[k]) || neighbors[k] == t))
					{
						neighbors[k]->came_from = currCell;
						neighbors[k]->score = score_temp;
						if(!is_in_evaluated_set(open_set, neighbors[k]))
							open_set->push_back(neighbors[k]);
					}
			}
			if(neighbors.size()==0)
			{
				currCell->noOfStalls+=1;

			}
			else
			{
				remove_from_open_set(open_set, currCell);
				closed_set->push_back(currCell);
			}
			neighbors.erase(neighbors.begin(), neighbors.end());

		}
		subRoutes->push_back(sr);
	}

	delete routingThisTS;
	delete sourceCells;
	delete targetCells;

}


//Remove the node from open set
void PrioritizedAStarRouter::remove_from_open_set(vector<PrStarCell*> *open_set, PrStarCell* current_node)
{
	for(unsigned i = 0; i< open_set->size(); ++i)
		if(((*open_set)[i])->x == current_node->x && ((*open_set)[i])->y == current_node->y)
		{
			open_set->erase(open_set->begin() + i);
		}
}

//Find the trace back path for droplets and add necessary stalls wherever possible
vector<RoutePoint*> * PrioritizedAStarRouter::find_path(PrStarCell* previous_node, PrStarCell* current_node)
{
	stack<PrStarCell*> stack_points;
	PrStarCell * next = current_node;
	vector<RoutePoint *> *sr = new vector<RoutePoint *>();
	while(next->x != previous_node->x || next->y != previous_node->y)
	{
		stack_points.push(next);
		next = next->came_from;
	}
	stack_points.push(next);
	while(!stack_points.empty())
	{
		while(stack_points.top()->noOfStalls-->=0)
		{
			RoutePoint* rp = new RoutePoint();
			rp->cycle = routeCycle;
			routeCycle++;
			rp->dStatus = DROP_NORMAL;
			rp->x = stack_points.top()->x;
			rp->y = stack_points.top()->y;
			sr->push_back(rp);

		}
		stack_points.pop();
	}
	return sr;
}

//Check the minimum node in the open set
PrStarCell* PrioritizedAStarRouter::score_min(vector<PrStarCell*> *curr_set, PrStarCell *t)
{
	PrStarCell* min = new PrStarCell();
	min->score = -1;
	int minH = -1;
	for(unsigned i = 0; i < curr_set->size(); ++i)
	{
		int h_score  = abs(t->x - (*curr_set)[i]->x) + abs(t->y - (*curr_set)[i]->y);
		if(min->score+minH > (*curr_set)[i]->score+h_score || min->score == -1)
		{
			min = (*curr_set)[i];
			minH = h_score;
		}
	}
	return min;
}

//Check if its in evaluated set
bool PrioritizedAStarRouter::is_in_evaluated_set(vector<PrStarCell*> *open_set, PrStarCell* in_question)
{
	for(unsigned i = 0; i< open_set->size(); ++i)
	if(((*open_set)[i])->x == in_question->x && ((*open_set)[i])->y == in_question->y)
	{
		return true;
	}
	return false;
}


int pointsInBoundingBox(map<Droplet *, PrStarCell *> *sourceCells, map<Droplet *, PrStarCell *> *targetCells, Droplet *d) {
	// Returns the number of points in  the bounding box for a given droplet's net
	//
	// The increasing 'Points in bounding box' priority scheme is based on the descriptions at
	// http://www.ece.umn.edu/~sachin/jnl/integration01.pdf and
	// http://users.ece.gatech.edu/limsk/course/ece6133/slides/multi-net.pdf

	// get coordinates of bounding box
	int d_sx = sourceCells->at(d)->x;
	int d_sy = sourceCells->at(d)->y;
	int d_tx = targetCells->at(d)->x;
	int d_ty = targetCells->at(d)->y;

	// initialize number of points accounting for the counting of this droplet's own source and sink
	int num_points = -2;

	// count sources in bounding box
	for (map<Droplet *, PrStarCell *>::iterator it = sourceCells->begin(); it != sourceCells->end(); it++) {
		if ( ( d_sx <= d_tx && d_sx <= it->second->x && it->second->x <= d_tx ) ||
			 ( d_sx >= d_tx && d_sx >= it->second->x && it->second->x >= d_tx ) ||
			 ( d_sy <= d_ty && d_sy <= it->second->y && it->second->y <= d_ty ) ||
			 ( d_sx >= d_ty && d_sy >= it->second->y && it->second->y >= d_ty ) )
			num_points++;
	}

	// count sinks in bounding box
	for (map<Droplet *, PrStarCell *>::iterator it = targetCells->begin(); it != sourceCells->end(); it++) {
		if ( ( d_sx <= d_tx && d_sx <= it->second->x && it->second->x <= d_tx ) ||
			 ( d_sx >= d_tx && d_sx >= it->second->x && it->second->x >= d_tx ) ||
			 ( d_sy <= d_ty && d_sy <= it->second->y && it->second->y <= d_ty ) ||
			 ( d_sx >= d_ty && d_sy >= it->second->y && it->second->y >= d_ty ) )
			num_points++;
	}

	return num_points;
};

struct pr_a_star_router_less_than {
	// Function object passed into std::sort to sort Droplets according to selected priority type

	map<Droplet *, PrStarCell *> *sc;
	map<Droplet *, PrStarCell *> *tc;
	RouterType t;

	pr_a_star_router_less_than(map<Droplet *, PrStarCell *> *sourceCells, map<Droplet *, PrStarCell *> *targetCells, RouterType type) {
		this->sc = sourceCells;
		this->tc = targetCells;
		this->t = type;
	}

	bool operator() (Droplet *d1, Droplet *d2) {

		if (t == PR_A_STAR_TYPE_1S_R || t == PR_A_STAR_TYPE_2S_R || t == PR_A_STAR_TYPE_3S_R) {
			// Order Droplets by increasing length (Manhattan distance between source and sink)
			return ( (abs(tc->at(d1)->x - sc->at(d1)->x) + abs(tc->at(d1)->y - sc->at(d1)->y)) <
				     (abs(tc->at(d2)->x - sc->at(d2)->x) + abs(tc->at(d2)->y - sc->at(d2)->y)) );
		}
		else if (t == PR_A_STAR_TYPE_1L_R || t == PR_A_STAR_TYPE_2L_R || t == PR_A_STAR_TYPE_3L_R) {
			// Order Droplets by decreasing length (Manhattan distance between source and sink)
			return ( (abs(tc->at(d1)->x - sc->at(d1)->x) + abs(tc->at(d1)->y - sc->at(d1)->y)) >
		             (abs(tc->at(d2)->x - sc->at(d2)->x) + abs(tc->at(d2)->y - sc->at(d2)->y)) );
		}
		else if (t == PR_A_STAR_TYPE_1X_R || t == PR_A_STAR_TYPE_2X_R || t == PR_A_STAR_TYPE_3X_R) {
			// Order Droplets by increasing X-component length, overall length used to break ties
			int delx_d1 = abs(tc->at(d1)->x - sc->at(d1)->x);
			int delx_d2 = abs(tc->at(d2)->x - sc->at(d2)->x);

			if (delx_d1 != delx_d2) {
				return ( delx_d1 < delx_d2 );
			}
			return ( (delx_d1 + abs(tc->at(d1)->y - sc->at(d1)->y)) < (delx_d2 + abs(tc->at(d2)->y - sc->at(d2)->y)) );
		}
		else if (t == PR_A_STAR_TYPE_1Y_R || t == PR_A_STAR_TYPE_2Y_R || t == PR_A_STAR_TYPE_3Y_R) {
			// Order Droplets by increasing Y-component length, overall length used to break ties
			int dely_d1 = abs(tc->at(d1)->x - sc->at(d1)->x);
			int dely_d2 = abs(tc->at(d2)->x - sc->at(d2)->x);

			if (dely_d1 != dely_d2) {
				return ( dely_d1 < dely_d2 );
			}
			return ( (abs(tc->at(d1)->x - sc->at(d1)->x) + dely_d1) < (abs(tc->at(d2)->y - sc->at(d2)->y) + dely_d2) );
		}
		else if (t == PR_A_STAR_TYPE_1B_R || t == PR_A_STAR_TYPE_2B_R || t == PR_A_STAR_TYPE_3B_R) {
			// Order Droplets by increasing number of points (sources and sinks from other nets) in its bounding box
			return ( pointsInBoundingBox(sc, tc, d1) < pointsInBoundingBox(sc, tc, d2) );
		}

	return true; // Return statement added to remove compile-time warnings, never executed
	};
};


void PrioritizedAStarRouter::reorderDropletsByPriority(map<Droplet*, PrStarCell*> *sC, map<Droplet*, PrStarCell*> *tC, vector<Droplet*> *rTTS) {

	if (type == PR_A_STAR_TYPE_1R_R || type == PR_A_STAR_TYPE_2R_R || type == PR_A_STAR_TYPE_3R_R)
		random_shuffle(rTTS->begin(), rTTS->end());
	else
		sort(rTTS->begin(), rTTS->end(), pr_a_star_router_less_than(sC, tC, type));

};


