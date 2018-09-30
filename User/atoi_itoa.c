
/* In this platform, 'int' takes 4 bytes */

/**
 * @param str ended with null (range 0 ~ 2G-1, 1048575)
 * @return conversion result / -1 if there is an error
 */
int m_atoi(char *str)
{
	int res=0, i=0;
	
	while(str[i] != 0)
	{
		if(str[i]<'0' || str[i]>'9')
			return -1;
		res = res*10 + str[i] - '0';
		i++;
	}
	return res;
}

/**
 * @param n number to be convered into str
 * @param str pointer to a buffer provided by caller
 * @param strlen length of str buffer, should >1
 * @return 0 if succeed(in this case, str is filled with the converted string ended with null) 
 *        -1 if there is an error
 */
int m_itoa(int n, char *str, int strlen)
{
	int len, i;
	int tmp;
	
	if(n == 0)
	{
		str[0] = '0';
		str[1] = 0;
		return 0;
	}
	
	len = 0;
	tmp = n;
	while(tmp)
	{
		len++;
		tmp /= 10;
	}
	if(len > strlen-1)
		return -1;
	
	i = 0;
	while(n)
	{
		str[len-1-i] = n%10 + '0';
		n /= 10;
		i++;
	}
	str[len] = 0;
	
	return 0;
}





