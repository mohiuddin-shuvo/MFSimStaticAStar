/*
 * A_Star_Router.h
 *
 *  Created on: Nov 15, 2014
 *  Authors: Mohiuddin Abdul Qader, Abhinand Menon
 */

#ifndef A_STAR_ROUTER_H
#define A_STAR_ROUTER_H


#include "../Testing/elapsed_timer.h"
#include "post_subprob_compact_router.h"


class AStarRouter : public PostSubproblemCompactionRouter
{
	public:
		// Constructors
		AStarRouter();
		AStarRouter(DmfbArch *dmfbArch);
		virtual ~AStarRouter();

	protected:
		void computeIndivSupProbRoutes(vector<vector<RoutePoint *> *> *subRoutes, vector<Droplet *> *subDrops, map<Droplet *, vector<RoutePoint *> *> *routes);
		void routerSpecificInits();

	private:

		// Methods
		vector<RoutePoint*> * find_path( DropletPoint* current_node);
		map<Droplet*, DropletPoint>* remove_from_open_set(vector<map<Droplet*, DropletPoint> *> *open_set, int i);
		int score_min(vector<map<Droplet*, DropletPoint>*> *curr_set, map<Droplet *, StarCell *> *targetCell);
		bool is_in_evaluated_set(map<Droplet *, DropletPoint>*, int* score);
		bool is_in_open_set(map<Droplet *, DropletPoint>*, int* score);
		bool checkGoalState(map<Droplet*, DropletPoint> *currCell, map<Droplet *, StarCell *> *targetCell);
		void find_neighbors(int dropletIndex, map<Droplet*, DropletPoint> currCell, map<Droplet *, StarCell *> *targetCell);
		void printBlockages();

		// Members
		vector<vector<StarCell *> *> *board;
		vector<map<Droplet*, DropletPoint>*> *open_set;
		vector<map<Droplet*, DropletPoint>*> *evaluated;
		vector<Droplet *> *routingThisTS;


};

#endif



