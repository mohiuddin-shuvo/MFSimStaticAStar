
/*
 * A_Star_Router.h
 *
 *  Created on: Nov 15, 2014
 *  Authors: Mohiuddin Abdul Qader, Abhinand Menon
 */


#include "../../Headers/Router/a_star_router.h"
#include <stack>
#include <set>



///////////////////////////////////////////////////////////////////////////////////
// Constructors
///////////////////////////////////////////////////////////////////////////////////
AStarRouter::AStarRouter()
{
	board = NULL;
	claim(false, "Invalid constructor used for Router variant.  Must use form that accepts DmfbArch.\n");
	open_set = NULL;
	evaluated = NULL;
	routingThisTS = NULL;
}
AStarRouter::AStarRouter(DmfbArch *dmfbArch)
{
	board = NULL;
	arch = dmfbArch;
	open_set = NULL;
	evaluated = NULL;
	routingThisTS = NULL;
}

void AStarRouter::printBlockages()
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
AStarRouter::~AStarRouter()
{
	//cerr << "Destructor" << endl;
	if (board)
	{
		while (!board->empty())
		{
			vector<StarCell*> *v = board->back();
			board->pop_back();
			while (!v->empty())
			{
				StarCell *c = v->back();
				v->pop_back();
				delete c;
			}
			delete v;
		}
		delete board;
	}
}

///////////////////////////////////////////////////////////////////////////////////
// Initializes the board full of StarCells instead of Soukup cells for Lee's
///////////////////////////////////////////////////////////////////////////////////
void AStarRouter::routerSpecificInits()
{
	// Create a 2D-array of Lee cells
	board = new vector<vector<StarCell *> *>();
	for (int x = 0; x < arch->getNumCellsX(); x++)
	{
		vector<StarCell *> *col = new vector<StarCell *>();
		for (int y = 0; y < arch->getNumCellsY(); y++)
		{
			StarCell *c = new StarCell();
			c->x = x;
			c->y = y;
			c->block = false;
			col->push_back(c);
		}
		board->push_back(col);
	}
}

///////////////////////////////////////////////////////////////////////////////////////
//Print the currnet A* Cell. Here a cell is a hash table of droplet to its route points
////////////////////////////////////////////////////////////////////////////////////////

static void printcurrstate(vector<Droplet *> *routingThisTS, map<Droplet*, DropletPoint> currCell, map<Droplet *, StarCell *> *targetCells)
{
	for(int i = 0; i < routingThisTS->size(); ++i)
	{

		Droplet *d = routingThisTS->at(i);
		DropletPoint t = currCell.at(d);
		StarCell* sc = targetCells->at(d);

		cout<< "Droplet "<<i<<": Current Postion: ("<<t.x<<","<<t.y<<")"<<endl;
		cout<< "Droplet "<<i<<": Goal Postion: ("<<sc->x<<","<<sc->y<<")"<<endl<<endl;

	}
}


void AStarRouter::computeIndivSupProbRoutes(vector<vector<RoutePoint *> *> *subRoutes, vector<Droplet *> *subDrops, map<Droplet *, vector<RoutePoint *> *> *routes)
{
	// Get the nodes that need to be routed
	vector<AssayNode *> routableThisTS;
	for (unsigned i = 0; i < thisTS->size(); i++)
		if (thisTS->at(i)->GetType() != DISPENSE && thisTS->at(i)->GetStartTS() == startingTS)
			routableThisTS.push_back(thisTS->at(i));

	// Gather the source and destination cells of each routable droplet this TS
	// For now, assume a non-io source is at the location of its last route point; non-io destination is bottom-left
	map<Droplet *, StarCell *> *sourceCells = new map<Droplet *, StarCell *>();
	map<Droplet *, StarCell *> *targetCells = new map<Droplet *, StarCell *>();
	set<ReconfigModule *> storageModsWithMultDrops;
	routingThisTS = new vector<Droplet *>();
	vector<StarCell*> blockCells;
	StarCell *s = NULL;
	StarCell *t = NULL;

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
			sourceCells->insert(pair<Droplet *, StarCell *>(pd, s));

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
			targetCells->insert(pair<Droplet *, StarCell *>(pd, t));
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
					StarCell *t1 = targetCells->at(d1);
					StarCell *t2 = targetCells->at(d2);

					if ((rp1->x == t2->x && rp1->y == t2->y) || (rp2->x == t1->x && rp2->y == t1->y))
					{
						targetCells->erase(d1);
						targetCells->erase(d2);
						targetCells->insert(pair<Droplet *, StarCell *>(d1, t2));
						targetCells->insert(pair<Droplet *, StarCell *>(d2, t1));
					}

				}
			}
		}


		if(routingThisTS->size()==0)
			return;
	open_set = new vector<map<Droplet*, DropletPoint>*>();
	evaluated = new vector<map<Droplet*, DropletPoint>*>();



	routeCycle = cycle;

	map<Droplet*, DropletPoint> *dtodp = new map<Droplet*, DropletPoint>();
    
    //Check for blocks for new reconfig areas

	for(unsigned i = 0; i < routingThisTS->size(); ++i)
	{

		Droplet *d = routingThisTS->at(i);
		DropletPoint dp;
		s = sourceCells->at(d);

		dp.camefrom.push_back(s);
		dp.droplet = d;
		dp.g_score = 0;
		dp.x = s->x;
		dp.y = s->y;
		dp.droplet = d;

		if(startingTS==12)
		{
			cout<<dp.x<<" : "<<dp.y<<endl;
		}
		dtodp->insert(pair<Droplet *, DropletPoint>(d, dp));

		StarCell *c = board->at(s->x)->at(s->y);
		c->isUsed = true;  // set marker for router types 2 and 3

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
	}




		map<Droplet*, DropletPoint> *currCell;
		currCell = dtodp;


		open_set->push_back(dtodp);
		//while loop to go through A*
		while(!open_set->empty())
		{
			int minIndex = score_min(open_set, targetCells);
			currCell = remove_from_open_set(open_set, minIndex);

			if(checkGoalState(currCell, targetCells) == true)
			{
				cout<<"goal\n";
				for(unsigned i = 0; i < routingThisTS->size(); ++i)
				{

					Droplet *d = routingThisTS->at(i);
					DropletPoint t = currCell->at(d);
					StarCell *c = board->at(t.x)->at(t.y);
					c->isUsed  = true;

					subRoutes->push_back(find_path(&t));
					subDrops->push_back(d);

				}

				break;
			}


			//Find all the neighbors
			map<Droplet *, DropletPoint> dtodp;
			for(unsigned i = 0; i < routingThisTS->size(); ++i)
			{

				Droplet *d = routingThisTS->at(i);
				dtodp.insert(pair<Droplet *, DropletPoint>(d, currCell->at(d)));

			}



			find_neighbors(0 , dtodp, targetCells);

			evaluated->push_back(currCell);
		}

	delete routingThisTS;
	delete sourceCells;
	delete targetCells;



}

//Find all the nighbors of the currnet A* cell, each droplet can go up, down, forward, back or stall, creating 5^d new cells for each cell
//Where d is number of droplets. Here recursively the funcion computes different movements for all the droplets

void AStarRouter::find_neighbors(int dropletIndex, map<Droplet*, DropletPoint> currCell, map<Droplet *, StarCell *> *targetCells)
{
	Droplet* d = routingThisTS->at(dropletIndex);
	DropletPoint dp = currCell.at(d);
	int cx = dp.x; // Current x,y
	int cy = dp.y;
	int shouldStall = 0;
	int blockWieght = 10;
	DropletPoint dpOriginal = currCell.at(d);

	 if(targetCells->at(d)->x == dp.x && targetCells->at(d)->y == dp.y)
	 {
		if(dropletIndex == routingThisTS->size()-1)
		{
			//Add to open set
			map<Droplet *, DropletPoint> *dtodp = new map<Droplet *, DropletPoint> ();
			int score = 0;
			for(unsigned i = 0; i <= dropletIndex; ++i)
			{

				Droplet *d1 = routingThisTS->at(i);
				DropletPoint dp1= currCell.at(d1);
				StarCell *c = board->at(dp1.x)->at(dp1.y);

				dtodp->insert(pair<Droplet *, DropletPoint>(d1, dp1));
				score += dp1.g_score;
				score += abs(targetCells->at(d1)->x - dp1.x);
				score += abs(targetCells->at(d1)->y - dp1.y);;
			}
			int eScore = 0;
			if(is_in_evaluated_set(dtodp, &eScore))
			{
			}
			else if(!is_in_open_set(dtodp, &eScore) || score < eScore)
			{
				open_set->push_back(dtodp);
			}



		}
		else
			find_neighbors(dropletIndex+1, currCell, targetCells );

		return;
	 }


	 dp = dpOriginal;
	 //grab the neighbor cells to find best possible
	if (cx+1 >= 0 && cx+1 < arch->getNumCellsX() && cy >= 0 && cy < arch->getNumCellsY()) // Right neighbor
	{
		//Find whether if its a block for other droplets
		StarCell *c = board->at(cx+1)->at(cy);
		{
			bool f = false;
			for (unsigned j = 0; j < routingThisTS->size(); j++)
			{
				Droplet *d1 = routingThisTS->at(j);
				DropletPoint dp1= currCell.at(d1);
				if(targetCells->at(d1)->x == dp1.x && targetCells->at(d1)->y == dp1.y)
					continue;
				if (j != dropletIndex)
				{
					StarCell *sc = board->at(dp1.x)->at(dp1.y);
					for (int x = sc->x-1; x <= sc->x+1; x++)
						for (int y = sc->y-1; y <= sc->y+1; y++)
							if (x >= 0 && y >= 0 && x < arch->getNumCellsX() && y < arch->getNumCellsY()) // On board?
								//if ( !(abs(x - s->x) <= 1 && abs(y - s->y) <= 1) ) // Don't block self
									//if ( !(abs(x - t->x) <= 1 && abs(y - t->y) <= 1) ) // Don't block self
										//if (!(*routes)[d2]->empty()) // Don't mark as blockage for dispense droplets; they wait in reservoir
										if(c->x == x && c->y == y)
										{
											f=true;

											break;
										}
				}
			}

			if(!f)  // No conflicts
			{
				dp.camefrom.push_back(board->at(cx)->at(cy));
				dp.x = c->x;
				dp.y = c->y;
				dp.g_score = dp.g_score+1;
				if(type==A_STAR_TYPE_2_R)
				{
					if(!c->isUsed)
					{
						dp.g_score += 10;
					}
				}
				else if(type==A_STAR_TYPE_3_R)
				{
					if(c->isUsed)
					{
						dp.g_score += 10;
					}
				}
				if(c->block == true)
					dp.g_score = blockWieght;
				currCell[d] = dp;

				if(dropletIndex == routingThisTS->size()-1)
				{
					//Add to open set
					map<Droplet *, DropletPoint> *dtodp = new map<Droplet *, DropletPoint> ();
					int score = 0;
					for(unsigned i = 0; i <= dropletIndex; ++i)
					{

						Droplet *d1 = routingThisTS->at(i);
						DropletPoint dp1= currCell.at(d1);
						StarCell *c = board->at(dp1.x)->at(dp1.y);

						dtodp->insert(pair<Droplet *, DropletPoint>(d1, dp1));
						score += dp1.g_score;
						score += abs(targetCells->at(d1)->x - dp1.x);
						score += abs(targetCells->at(d1)->y - dp1.y);;
					}
					int eScore = 0;
					if(is_in_evaluated_set(dtodp, &eScore))
					{
					}
					else if(!is_in_open_set(dtodp, &eScore) || score < eScore)
					{
						open_set->push_back(dtodp);
					}



				}
				else
				{
					// recursive call for next droplet

					find_neighbors(dropletIndex+1, currCell, targetCells );
				}
			}
			else
				shouldStall++;

		}
	}
	dp = dpOriginal;
	if (cx-1 >= 0 && cx-1 < arch->getNumCellsX() && cy >= 0 && cy < arch->getNumCellsY()) // Left neighbor
	{
			//Find whether if its a block for other droplets
			StarCell *c = board->at(cx-1)->at(cy);
			{
				bool f = false;
				for (unsigned j = 0; j < routingThisTS->size(); j++)
							{
								Droplet *d1 = routingThisTS->at(j);
								DropletPoint dp1= currCell.at(d1);
								if(targetCells->at(d1)->x == dp1.x && targetCells->at(d1)->y == dp1.y)
									continue;
								if (j != dropletIndex)
								{
									StarCell *sc = board->at(dp1.x)->at(dp1.y);
									for (int x = sc->x-1; x <= sc->x+1; x++)
										for (int y = sc->y-1; y <= sc->y+1; y++)
											if (x >= 0 && y >= 0 && x < arch->getNumCellsX() && y < arch->getNumCellsY()) // On board?
														if(c->x == x && c->y == y)
														{
															f=true;

															break;
														}
								}
							}
				if(!f)// No conflicts
				{
					dp.camefrom.push_back(board->at(cx)->at(cy));
					dp.x = c->x;
					dp.y = c->y;
					dp.g_score = dp.g_score+1;
					if(type==A_STAR_TYPE_2_R)
					{
						if(!c->isUsed)
						{
							dp.g_score += 10;
						}
					}
					else if(type==A_STAR_TYPE_3_R)
					{
						if(c->isUsed)
						{
							dp.g_score += 10;
						}
					}
					if(c->block == true)
						dp.g_score = blockWieght;
					currCell[d] = dp;

					if(dropletIndex >= routingThisTS->size()-1)
					{
						//Add to open set
						map<Droplet *, DropletPoint> *dtodp = new map<Droplet *, DropletPoint> ();
						int score = 0;
						for(unsigned i = 0; i <= dropletIndex; ++i)
						{

							Droplet *d1 = routingThisTS->at(i);
							DropletPoint dp1= currCell.at(d1);
							StarCell *c = board->at(dp1.x)->at(dp1.y);

							dtodp->insert(pair<Droplet *, DropletPoint>(d1, dp1));
							score += dp1.g_score;
							score += abs(targetCells->at(d1)->x - dp1.x);
							score += abs(targetCells->at(d1)->y - dp1.y);;
						}
						int eScore = 0;
						if(is_in_evaluated_set(dtodp, &eScore))
						{
						}
						else if(!is_in_open_set(dtodp, &eScore) || score < eScore)
						{
							open_set->push_back(dtodp);
						}



					}
					else
					{
						//call recursive for next droplet

						find_neighbors(dropletIndex+1, currCell, targetCells );
					}
				}
				else
					shouldStall++;

			}
	}
	dp = dpOriginal;
	if (cx >= 0 && cx < arch->getNumCellsX() && cy+1 >= 0 && cy+1 < arch->getNumCellsY()) // Bottom neighbor
	{
			//Find whether if its a block for other droplets
			StarCell *c = board->at(cx)->at(cy+1);
			{
				bool f = false;
				for (unsigned j = 0; j < routingThisTS->size(); j++)
							{
								Droplet *d1 = routingThisTS->at(j);
								DropletPoint dp1= currCell.at(d1);
								if(targetCells->at(d1)->x == dp1.x && targetCells->at(d1)->y == dp1.y)
									continue;
								if (j != dropletIndex)
								{
									StarCell *sc = board->at(dp1.x)->at(dp1.y);
									for (int x = sc->x-1; x <= sc->x+1; x++)
										for (int y = sc->y-1; y <= sc->y+1; y++)
											if (x >= 0 && y >= 0 && x < arch->getNumCellsX() && y < arch->getNumCellsY()) // On board?
														if(c->x == x && c->y == y)
														{
															f=true;

															break;
														}
								}
							}
				if(!f)// No conflicts
				{
					dp.camefrom.push_back(board->at(cx)->at(cy));
					dp.x = c->x;
					dp.y = c->y;
					dp.g_score = dp.g_score+1;
					if(type==A_STAR_TYPE_2_R)
					{
						if(!c->isUsed)
						{
							dp.g_score += 10;
						}
					}
					else if(type==A_STAR_TYPE_3_R)
					{
						if(c->isUsed)
						{
							dp.g_score += 10;
						}
					}
					if(c->block == true)
						dp.g_score = blockWieght;
					currCell[d] = dp;
					if(dropletIndex == routingThisTS->size()-1)
					{
						//Add to open set
						map<Droplet *, DropletPoint> *dtodp = new map<Droplet *, DropletPoint> ();
						int score = 0;
						for(unsigned i = 0; i <= dropletIndex; ++i)
						{

							Droplet *d1 = routingThisTS->at(i);
							DropletPoint dp1= currCell.at(d1);
							StarCell *c = board->at(dp1.x)->at(dp1.y);

							dtodp->insert(pair<Droplet *, DropletPoint>(d1, dp1));
							score += dp1.g_score;
							score += abs(targetCells->at(d1)->x - dp1.x);
							score += abs(targetCells->at(d1)->y - dp1.y);;
						}
						int eScore = 0;
						if(is_in_evaluated_set(dtodp, &eScore))
						{
						}
						else if(!is_in_open_set(dtodp, &eScore) || score < eScore)
						{
							open_set->push_back(dtodp);
						}

					}
					else
					{
						//call recursive for next droplet

						find_neighbors(dropletIndex+1, currCell, targetCells );
					}
				}
				else
					shouldStall++;
			}
	}
	dp = dpOriginal;
	if (cx >= 0 && cx < arch->getNumCellsX() && cy-1 >= 0 && cy-1 < arch->getNumCellsY()) // Top neighbor
	{
			//Find whether if its a block for other droplets
			StarCell *c = board->at(cx)->at(cy-1);
			{
				bool f = false;
				for (unsigned j = 0; j < routingThisTS->size(); j++)
							{
								Droplet *d1 = routingThisTS->at(j);
								DropletPoint dp1= currCell.at(d1);
								if(targetCells->at(d1)->x == dp1.x && targetCells->at(d1)->y == dp1.y)
									continue;
								if (j != dropletIndex)
								{
									StarCell *sc = board->at(dp1.x)->at(dp1.y);
									for (int x = sc->x-1; x <= sc->x+1; x++)
										for (int y = sc->y-1; y <= sc->y+1; y++)
											if (x >= 0 && y >= 0 && x < arch->getNumCellsX() && y < arch->getNumCellsY()) // On board?
														if(c->x == x && c->y == y)
														{
															f=true;

															break;
														}
								}
							}
				if(!f)// No conflicts
				{
					dp.camefrom.push_back(board->at(cx)->at(cy));
					dp.x = c->x;
					dp.y = c->y;
					dp.g_score = dp.g_score+1;
					if(type==A_STAR_TYPE_2_R)
					{
						if(!c->isUsed)
						{
							dp.g_score += 10;
						}
					}
					else if(type==A_STAR_TYPE_3_R)
					{
						if(c->isUsed)
						{
							dp.g_score += 10;
						}
					}
					if(c->block == true)
						dp.g_score = blockWieght;
					currCell[d] = dp;
					if(dropletIndex == routingThisTS->size()-1)
					{
						//Add to open set
						map<Droplet *, DropletPoint> *dtodp = new map<Droplet *, DropletPoint> ();
						int score = 0;
						for(unsigned i = 0; i <= dropletIndex; ++i)
						{

							Droplet *d1 = routingThisTS->at(i);
							DropletPoint dp1= currCell.at(d1);
							StarCell *c = board->at(dp1.x)->at(dp1.y);

							dtodp->insert(pair<Droplet *, DropletPoint>(d1, dp1));
							score += dp1.g_score;
							score += abs(targetCells->at(d1)->x - dp1.x);
							score += abs(targetCells->at(d1)->y - dp1.y);;
						}
						int eScore = 0;
						if(is_in_evaluated_set(dtodp, &eScore))
						{
						}
						else if(!is_in_open_set(dtodp, &eScore) || score < eScore)
						{
							open_set->push_back(dtodp);
						}

					}
					else
					{
						//call recursive for next droplet

						find_neighbors(dropletIndex+1, currCell, targetCells );
					}
				}
				else
					shouldStall++;


			}

	}
	dp = dpOriginal;
    //Add no movement also as a possible transition
	//if(shouldStall==4)
	{

		StarCell *sc = board->at(dp.x)->at(dp.y);
		dp.camefrom.push_back(sc);

		dp.g_score = dp.g_score+1;
		if(type==A_STAR_TYPE_2_R)
		{
			if(!sc->isUsed)
			{
				dp.g_score += 10;
			}
		}
		else if(type==A_STAR_TYPE_3_R)
		{
			if(sc->isUsed)
			{
				dp.g_score += 10;
			}
		}
		if(sc->block == true)
			dp.g_score = blockWieght;
		currCell[d] = dp;
		if(dropletIndex == routingThisTS->size()-1)
		{
			//Add to open set
			map<Droplet *, DropletPoint> *dtodp = new map<Droplet *, DropletPoint> ();
			int score = 0;
			for(unsigned i = 0; i <= dropletIndex; ++i)
			{

				Droplet *d1 = routingThisTS->at(i);
				DropletPoint dp1= currCell.at(d1);
				StarCell *c = board->at(dp1.x)->at(dp1.y);

				dtodp->insert(pair<Droplet *, DropletPoint>(d1, dp1));
				score += dp1.g_score;
				score += abs(targetCells->at(d1)->x - dp1.x);
				score += abs(targetCells->at(d1)->y - dp1.y);;
			}
			int eScore = 0;
			if(is_in_evaluated_set(dtodp, &eScore))
			{
			}
			else if(!is_in_open_set(dtodp, &eScore) || score < eScore)
			{
				open_set->push_back(dtodp);
			}

		}
		else
		{
			//call recursive for next droplet

			find_neighbors(dropletIndex+1, currCell, targetCells );
		}

	}

}

//Check for gaol state for a* algo, whether all droplets reach their destination
bool AStarRouter::checkGoalState(map<Droplet*, DropletPoint> *currCell, map<Droplet *, StarCell *> *targetCell)
{
	int i = 0;
	bool f = true;
	for( map<Droplet*, DropletPoint>::const_iterator it = currCell->begin(); it != currCell->end(); ++it )
	{

	  Droplet* d = it->first;
	  DropletPoint dp = it->second;

	 i++;
	  if(!(targetCell->at(d)->x == dp.x && targetCell->at(d)->y == dp.y))
	  {
		  f= false;

	  }

	}
	return f;

}

//Remove item from openset
map<Droplet*, DropletPoint> *AStarRouter::remove_from_open_set(vector<map<Droplet*, DropletPoint> *> *open_set, int i)
{
	map<Droplet*, DropletPoint> * currCell = open_set->at(i);
	open_set->erase(open_set->begin() + i);
	return currCell;
}

//Find the route paths and add it to subroute
vector<RoutePoint*> * AStarRouter::find_path( DropletPoint* current_node)
{
	int j = routeCycle;
	vector<RoutePoint *> *sr = new vector<RoutePoint *>();
	int prevx = 0;
	int prevy = 0;
	int i;
	for(i = 1; i<current_node->camefrom.size();i++)
	{
		StarCell *s = current_node->camefrom.at(i);
		RoutePoint* rp = new RoutePoint();
		rp->cycle = j;
		if(i!=1 && prevx == s->x && prevy == s->y)
		{
			rp->dStatus = DROP_INT_WAIT;

		}
		else
		{

			rp->dStatus = DROP_NORMAL;
		}
		j++;
		rp->x = s->x;
		rp->y = s->y;
		sr->push_back(rp);
		prevx = s->x;
		prevy = s->y;
	}
	StarCell *s = board->at(current_node->x)->at(current_node->y);;
	RoutePoint* rp = new RoutePoint();
	rp->cycle = j;
	j++;
	if(i!=1 && prevx == s->x && prevy == s->y)
	{
		rp->dStatus = DROP_INT_WAIT;

	}
	else
	{

		rp->dStatus = DROP_NORMAL;
	}
	rp->x = s->x;
	rp->y = s->y;
	sr->push_back(rp);
	return sr;
}


//Return the min score cell of the open set in A*
int AStarRouter::score_min(vector<map<Droplet*, DropletPoint>*> *curr_set, map<Droplet *, StarCell *> *targetCell )
{

	int minScore = -1;
	int minIndex = 0;

	for(unsigned i = 0; i < curr_set->size(); ++i)
	{
		map<Droplet*, DropletPoint>* node = curr_set->at(i);

		int manhattanDistance = 0;
		int g_score = 0;
		int f_score = 0;
		for( map<Droplet*, DropletPoint>::const_iterator it = node->begin(); it != node->end(); ++it )
		{
		  Droplet* d = it->first;
		  DropletPoint dp = it->second;
		  manhattanDistance+= abs(targetCell->at(d)->x - dp.x);
		  manhattanDistance+= abs(targetCell->at(d)->y - dp.y);

		  g_score += dp.g_score;

		}

		f_score = g_score+manhattanDistance;

		if(minScore > f_score || minScore == -1)
		{
			minScore = f_score;
			minIndex = i;
		}

	}
	if(startingTS == 19)
		cout<<minScore<<endl<<open_set->size()<<endl;
	return minIndex;
}
//Check whether in evaluated set
bool AStarRouter::is_in_evaluated_set(map<Droplet *, DropletPoint> *in_question, int* score)
{
	for(int i = 0; i< evaluated->size(); ++i)
	{
		bool f = false;

		map<Droplet *, DropletPoint> *in_ev;
		in_ev = evaluated->at(i);
		for(int j = 0; j< routingThisTS->size(); ++j)
		{
			Droplet *d1 = routingThisTS->at(j);
			DropletPoint dp1= in_question->at(d1);
			DropletPoint dp2= in_ev->at(d1);
			if(dp1.x != dp2.x || dp1.y != dp2.y)
			{
				f = true;
				break;
			}

		}
		if(!f)
		{
			return true;
		}
	}

	return false;
}
//Check whether in open set
bool AStarRouter::is_in_open_set(map<Droplet *, DropletPoint> *in_question, int* score)
{
	for(int i = 0; i< open_set->size(); ++i)
	{
		bool f = false;
		map<Droplet *, DropletPoint> *in_ev;
		in_ev = open_set->at(i);
		for(int j = 0; j< routingThisTS->size(); ++j)
		{
			Droplet *d1 = routingThisTS->at(j);
			DropletPoint dp1= in_question->at(d1);
			DropletPoint dp2= in_ev->at(d1);

			if(dp1.x != dp2.x || dp1.y != dp2.y)
			{
				f = true;
				break;
			}

		}
		if(!f)
		{
			return true;
		}
	}

	return false;
}
