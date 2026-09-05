# include "codexion.h"

void	coffman_circular_wait_breaker(t_coder *coder)
{
	if (coder->id % 2 == 0)
	{
		coder->first = coder->left_dongle;
		coder->second = coder->right_dongle;
	}
	else {
		coder->first = coder->right_dongle;
		coder->second = coder->left_dongle;
	}
}
