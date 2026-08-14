void	*coder_thread(void *arg)
{
	t_coder	*a_coder;

	a_coder = (t_coder *)arg;
	while (1)
	{
		lock_dongles(a_coder);
		request_dongles(a_coder);
		if (queue_is_first(a_coder->left, a_coder)
			&& queue_is_first(a_coder->right, a_coder))
		{
			if (dongles_ready(a_coder) == 1)
			{
				if (compile_cycle(a_coder) == 0)
					break;
			}
			else
				wait_dongles(a_coder);
		}
		else
		{
			unlock_dongles(a_coder);
			usleep(1000);
		}
	}
	return (NULL);
}