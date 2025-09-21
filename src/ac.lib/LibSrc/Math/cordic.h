
struct _CORDIC_Table {
	int		size;
	int		mval;
	double	kval;
	char	*index;
	double	*coeff;
};

typedef struct _CORDIC_Table CORDIC_Table;

