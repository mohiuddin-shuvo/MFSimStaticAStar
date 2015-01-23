/*
 * prioritized_a_star_router.h
 *
 *  Created on: Dec 2, 2014
 *  Authors: Mohiuddin Abdul Qader, Abhinand Menon
 */

#ifndef HEADERS_ROUTER_PRIORITIZED_A_STAR_ROUTER_H_
#define HEADERS_ROUTER_PRIORITIZED_A_STAR_ROUTER_H_


#include "../Testing/elapsed_timer.h"
#include "post_subprob_compact_router.h"


class PrioritizedAStarRouter : public PostSubproblemCompactionRouter
{
	public:
		// Constructors
	    PrioritizedAStarRouter();
	    PrioritizedAStarRouter(DmfbArch *dmfbArch);
		virtual ~PrioritizedAStarRouter();

	protected:
		void computeIndivSupProbRoutes(vector<vector<RoutePoint *> *> *subRoutes, vector<Droplet *> *subDrops, map<Droplet *, vector<RoutePoint *> *> *routes);
		void routerSpecificInits();

	private:
		// Methods
		vector<RoutePoint*> * find_path(PrStarCell* previous_node, PrStarCell* current_node);
		PrStarCell*  score_min(vector<PrStarCell*> *curr_set, PrStarCell *t);
		bool is_in_evaluated_set(vector<PrStarCell*> *open_set, PrStarCell* in_question);
		void remove_from_open_set(vector<PrStarCell*> *open_set, PrStarCell* current_node);
		void printBlockages();
		void reorderDropletsByPriority(map<Droplet *, PrStarCell *> *sourceCells, map<Droplet *, PrStarCell *> *targetCells, vector<Droplet *> *routingThisTS);

		// Member
		vector<vector<PrStarCell *> *> *board;


};



#endif /* HEADERS_ROUTER_PRIORITIZED_A_STAR_ROUTER_H_ */
