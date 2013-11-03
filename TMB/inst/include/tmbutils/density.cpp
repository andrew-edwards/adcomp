/**
   \file density.cpp
   Classes to construct multivariate Gaussian density objects.
*/


#define TYPEDEFS(scalartype_)			\
public:						\
typedef scalartype_ scalartype;			\
typedef vector<scalartype> vectortype;		\
typedef matrix<scalartype> matrixtype;		\
typedef array<scalartype> arraytype;

#define VARIANCE_NOT_YET_IMPLEMENTED vectortype variance(){};

/** \brief Multivariate normal distribution with unstructered covariance matrix

    Class to evaluate the negative log density of a mean zero
    multivariate Gaussian variable with general covariance matrix Sigma.
    Intended for small dense covariance matrices.
*/
template <class scalartype_>
class MVNORM_t{
  TYPEDEFS(scalartype_);
  scalartype logdetQ;
  /** Lower cholesky of _covariance_ */
  matrixtype L; 
  matrixtype Sigma; /* Keep for convenience - not used */
public:
  MVNORM_t(){}
  MVNORM_t(matrixtype Sigma_){
    setSigma(Sigma_);
  }

  /** \brief Covariance extractor */
  matrixtype cov(){return Sigma;}

  /* Lower triangular L: A=LL' */
  matrixtype chol(const matrixtype &a){
    int n=a.rows();
    matrixtype l(n,n);
    l.setZero();
    scalartype tmp;
    for(int i=0;i<n;i++)
      for(int j=0;j<=i;j++){
	tmp=a(i,j);
	for(int k=0;k<j;k++)tmp-=l(i,k)*l(j,k);
	if(i==j)l(i,j)=sqrt(tmp);
	else l(i,j)=tmp/l(j,j);
      }
    return l;
  }
  /* Find x:=solve(L,y) */
  vectortype lsolve(matrixtype &l, 
		    vectortype y){
    int n=l.rows();
    vectortype x(n);
    scalartype tmp;
    for(int i=0;i<n;i++){
      tmp=y(i);
      for(int k=0;k<i;k++)tmp-=x(k)*l(i,k);
      x(i)=tmp/l(i,i);
    }
    return x;
  }
  /* Find x:=solve(LL',y) - array case */
  arraytype lltsolve(matrixtype &l, 
		     arraytype y){
    int n=l.rows();
    arraytype x(y);
    arraytype tmp(y.col(0).dim);
    for(int i=0;i<n;i++){
      tmp=y.col(i);
      for(int k=0;k<i;k++)tmp=tmp-x.col(k)*l(i,k);
      x.col(i)=tmp/l(i,i);
    }
    /* Find x:=solve(t(L),x) */
    y=x;
    for(int i=n-1;i>=0;i--){
      tmp=y.col(i);
      for(int k=n-1;k>i;k--)tmp=tmp-x.col(k)*l(k,i);
      x.col(i)=tmp/l(i,i);
    }
    return x;
  }

  /* initializer via covariance matrix */
  void setSigma(matrixtype Sigma_){
    Sigma=Sigma_;
    L=chol(Sigma_);
    logdetQ=scalartype(0);
    for(int i=0;i<L.rows();i++)logdetQ -= scalartype(2)*log(L(i,i));
  }
  scalartype Quadform(vectortype x){
    vectortype u=lsolve(L,x);
    return (u*u).sum();
  }
  /** \brief Evaluate the negative log density */
  scalartype operator()(vectortype x){
    return -scalartype(.5)*logdetQ + scalartype(.5)*Quadform(x) + x.size()*scalartype(log(sqrt(2.0*M_PI)));
  }
  arraytype jacobian(arraytype x){
    return lltsolve(L,x);
  }
  int ndim(){return 1;}
  VARIANCE_NOT_YET_IMPLEMENTED;
};
template <class scalartype>
MVNORM_t<scalartype> MVNORM(matrix<scalartype> x){
  return MVNORM_t<scalartype>(x);
}

/** \brief Multivariate normal distribution with unstructered correlation matrix

   Class to evaluate the negative log density of a multivariate Gaussian 
   variable with unstructured symmetric positive definite correlation matrix.

   The unstructured correlation matrix is parameterized via a lower triangular matrix
   with unit diagonal i.e. (n*n-n)/2 parameters to describe an n dimensional correlation matrix.

   For instance in the case n=4 the correlation matrix is given by
   \f[\Sigma = D^{-\frac{1}{2}}LL'D^{-\frac{1}{2}}\f]
   where
   \f[
   L=\begin{pmatrix}
   1 \\
   x_0 & 1 \\
   x_1 & x_3 & 1 \\
   x_2 & x_4 & x_5 & 1
   \end{pmatrix}
   \f]
   and
   \f[
   D=diag(LL')
   \f]

   Example:
   \code
   // Construct density object of dimension 4
   vector<Type> Lx(6);
   UNSTRUCTURED_CORR_t<Type> neg_log_density(Lx);
   // Evaluate density
   vector<Type> x(4);
   Type ans=neg_log_density(x);
   \endcode

   \remark The correlation matrix is available through member "Sigma".
*/   
template <class scalartype_>
class UNSTRUCTURED_CORR_t : public MVNORM_t<scalartype_>{
  TYPEDEFS(scalartype_);
  UNSTRUCTURED_CORR_t(){}
  UNSTRUCTURED_CORR_t(vectortype x){
    // (n*n-n)/2=nx  ==>  n*n-n-2*nx=0 ==> n=(1+sqrt(1+8*nx))/2
    int nx=x.size();
    int n=int((1.0+sqrt(1+8*nx))/2.0);
    if((n*n-n)/2!=nx)std::cout << "vector does not specify an UNSTRUCTERED_CORR\n";
    matrixtype L(n,n);
    L.setIdentity();
    int i,j,k=0;
    for(i=0;i<L.rows();i++){
      for(j=0;j<L.cols();j++){
	if(i>j){L(i,j)=x[k];k++;}
      }
    }
    matrixtype llt=L*L.transpose();
    matrixtype Sigma=llt;
    for(i=0;i<Sigma.rows();i++){
      for(j=0;j<Sigma.cols();j++){
	Sigma(i,j)/=sqrt(llt(i,i)*llt(j,j));
      }
    }    
    this->setSigma(Sigma); /* Call MVNORM_t initializer */
  }
};
template <class scalartype>
UNSTRUCTURED_CORR_t<scalartype> UNSTRUCTURED_CORR(vector<scalartype> x){
  return UNSTRUCTURED_CORR_t<scalartype>(x);
}

/** \brief Standardized normal distribution

    Class to evaluate the negative log density of a (multivariate)
    standard normal distribution.
    \verbatim
    Examples: N01()
    \endverbatim
*/   
template<class scalartype_> 
class N01{
  TYPEDEFS(scalartype_);
public:
  /** \brief Evaluate the negative log density */
  scalartype operator()(array<scalartype> x){
    return (x*x*.5 + log(sqrt(2.0*M_PI)) ).sum() ;
  }
  arraytype jacobian(arraytype x){return x;}
  int ndim(){return 1;}
  VARIANCE_NOT_YET_IMPLEMENTED;
};

/** \brief Stationary AR1 process

    Class to evaluate the negative log density of a (multivariate) AR1
    process with parameter phi and given marginal distribution.
    @param phi       Scalar -1<phi<1
    @tparam MARGINAL The desired (multivariate) marginal distribution.

    Let \f$f(x)\f$ denote a multivariate Gaussian mean-zero negative log density
    represented by its covariance matrix \f$\Sigma\f$. Define recursively the vectors
    \f[x_0\sim N(0,\Sigma)\f]
    \f[x_1 = \phi x_0 + \varepsilon_1\:,\:\:\: \varepsilon_1 \sim N(0,\sigma\Sigma)\f]
    \f[x_i = \phi x_{i-1} + \varepsilon_i\:,\:\:\: \varepsilon_i \sim N(0,\sigma\Sigma)\f]
    where \f$\sigma=\sqrt{1-\phi^2}\f$. Then \f$E(x_i)=0\f$, \f$V(x_i)=\Sigma\f$ and the covariance
    is \f$E(x_ix_j')=\phi^{|i-j|}\Sigma\f$. We refer to this process as a stationary 1st order
    autoregressive process with multivariate increments with parameter phi and marginal distribution f.
    Compactly denoted AR1(phi,f).

    Note that the construction can be carried out recursively, as "AR1(phi,f)" is itself a distribution
    that can be used as input to AR1(). See example below:
    \code
    \\ Construct negative log density of standard AR1 process on a line:
    Type phi1=0.8;
    AR1_t<N01<Type> > f1(phi1);
    \\ Can be evaluated on a vector:
    vector<Type> x(10);
    Type ans=f1(x);
    \endcode

    Now use f1 as marginal in a new AR1 process with parameter phi2:

    \code
    \\ Construct negative log density of standard AR1 process on a line:
    Type phi2=0.5;
    AR1_t<AR1_t<N01<Type> > > f2(phi1,f1);
    \\ Can be evaluated on a 2-dimensional array:
    vector<Type> x(10,20);
    Type ans=f2(x);
    \endcode

*/   
template <class distribution>
class AR1_t{
  TYPEDEFS(typename distribution::scalartype);
private:
  scalartype phi;
  distribution MARGINAL;
public:
  AR1_t(){/*phi=phi_;MARGINAL=f_;*/}
  AR1_t(scalartype phi_, distribution f_){phi=phi_;MARGINAL=f_;}
  /** \brief Evaluate the negative log density */
  scalartype operator()(vectortype x){
    scalartype value;
    value=scalartype(0);
    int n=x.rows();
    int m=x.size()/n;
    scalartype sigma=sqrt(scalartype(1)-phi*phi); /* Steady-state standard deviation */
    value+=MARGINAL(x(0));                                       /* E.g. x0 ~ N(0,1)  */
    for(int i=1;i<n;i++)value+=MARGINAL((x(i)-x(i-1)*phi)/sigma);/* x(i)-phi*x(i-1) ~ N(0,sigma^2) */
    value+=scalartype((n-1)*m)*log(sigma);
    return value;
  }

  /* Copied vector version - apply over outermost dimension */
  scalartype operator()(arraytype x){
    scalartype value;
    value=scalartype(0);
    int n=x.cols();
    int m=x.size()/n;
    scalartype sigma=sqrt(scalartype(1)-phi*phi); /* Steady-state standard deviation */
    value+=MARGINAL(x.col(0));                                  /* E.g. x0 ~ N(0,1)  */
    for(int i=1;i<n;i++)value+=MARGINAL((x.col(i)-x.col(i-1)*phi)/sigma);/* x(i)-phi*x(i-1) ~ N(0,sigma^2) */
    value+=scalartype((n-1)*m)*log(sigma);
    return value;
  }


  arraytype jacobian(arraytype x){
    scalartype sigma=sqrt(scalartype(1)-phi*phi); /* Steady-state standard deviation */
    int n=x.cols();
    arraytype y(x.dim);
    y.setZero();
    y.col(0) = y.col(0) + MARGINAL.jacobian(x.col(0));
    for(int i=1;i<n;i++){
      //MARGINAL((x(i)-x(i-1)*phi)/sigma);
      y.col(i-1) = y.col(i-1) - (phi/sigma) * MARGINAL.jacobian((x.col(i)-x.col(i-1)*phi)/sigma);
      y.col(i) = y.col(i) +  MARGINAL.jacobian((x.col(i)-x.col(i-1)*phi)/sigma)/sigma;
    }
    return y;
  }
  int ndim(){return 1;}
  VARIANCE_NOT_YET_IMPLEMENTED;
};
template <class scalartype, class distribution>
AR1_t<distribution> AR1(scalartype phi_, distribution f_){
  return AR1_t<distribution>(phi_,f_);
}
template <class scalartype>
AR1_t<N01<scalartype> > AR1(scalartype phi_){
  return AR1_t<N01<scalartype> >(phi_,N01<scalartype>());
}


/** \brief Stationary AR(k) process.

    @param phi_ Vector of length k with parameters.

    \verbatim
    Class to evaluate the negative log density of a stationary 
    AR(k)-process with parameter vector phi=[phi_1,...,phi_k]:
    
    x[t]=phi_1*x[t-1]+...+phi_k*x[t-k]+eps[t]

    where eps[t]~N(0,sigma^2). The parameter sigma^2 is chosen to 
    obtain V(x[t])=1 so that the class actually specifies a correlation
    model.

    Examples: ARk(phi)  <-- simple mean zero variance 1 AR(k) process.

    Steady state initial distribution is found by (e.g. k=3)

    [gamma(1)]    [gamma(0) gamma(1) gamma(2)]     [phi1]
    [ ....   ] =  [gamma(1) gamma(0) gamma(1)]  *  [phi2]
    [gamma(3)]    [gamma(2) gamma(1) gamma(0)]     [phi3]
    \endverbatim

*/   
template <class scalartype_>
class ARk_t{
  TYPEDEFS(scalartype_);
  //private:
  int k;
  vectortype phi;   /* [phi1,...,phik] */
  vectortype gamma; /* [gamma(1),...,gamma(k)] (note gamma(0) is 1) */
  /* Initial distribution matrices. */
  matrixtype V0;    /* kxk variance  */
  matrixtype Q0;    /* kxk precision */
  /* gamma is found through (I-M)*gamma=phi ... */
  matrixtype M;     /* kxk   */
  matrixtype I;     /* kxk   */
  scalartype sigma;/* increment standard deviation */
  scalartype logdetQ0;
public:
  ARk_t(){/*phi=phi_;MARGINAL=f_;*/}
  ARk_t(vectortype phi_){
    phi=phi_;
    k=phi.size();
    V0.resize(k,k);Q0.resize(k,k);
    M.resize(k,k);I.resize(k,k);
    /* build M-matrix */
    M.setZero();
    int d;
    for(int i=0;i<k;i++){
      for(int j=0;j<k;j++){
	d=abs(i-j);
	if(d!=0){
	  M(i,d-1)+=phi[j];
	}
      }
    }
    I.setIdentity();
    gamma=((I-M).inverse())*matrixtype(phi);
    /* Increment sd */
    sigma=sqrt(scalartype(1)-(phi*gamma).sum());
    /* build V0 matrix */
    for(int i=0;i<k;i++){
      for(int j=0;j<k;j++){
	d=abs(i-j);
	if(d==0)
	  V0(i,j)=scalartype(1); 
	else 
	  V0(i,j)=gamma(d-1);
      }
    }
    /* build Q0 matrix */
    Q0=V0.inverse();
    /* log determinant */
    matrixtype L=Q0.llt().matrixL(); /*L L' = Q*/
    logdetQ0=scalartype(0);
    for(int i=0;i<k;i++)logdetQ0+=scalartype(2)*log(L(i,i));
  }
  /** \brief Covariance extractor. 
      Run Youle-Walker recursions and return a vector of length n representing
      the auto-covariance function.
  */
  vectortype cov(int n){
    vectortype rho(n);
    for(int i=0;i<n;i++){
      if(i==0){rho(0)=scalartype(1);}
      else if(i<=k){rho(i)=gamma(i-1);}
      else { /* youle walker */
	scalartype tmp=0;
	for(int j=0;j<k;j++)tmp+=phi[j]*rho[i-1-j];
	rho(i)=tmp;
      }
    }
    return rho;
  }

  /** \brief Evaluate the negative log density */
  scalartype operator()(vectortype x){
    if(x.size()<k)std::cout << "AR(k) density requires vector length at least k\n";
    scalartype value=0;
    for(int i=0;i<k;i++)
      for(int j=0;j<k;j++)
	value+=scalartype(.5)*x[i]*Q0(i,j)*x[j];
    value-=scalartype(.5)*(logdetQ0- k*scalartype(log(2.0*M_PI)) );
    scalartype tmp;
    for(int i=k;i<x.size();i++){
      tmp=scalartype(0);
      for(int j=0;j<k;j++){
	tmp+=phi[j]*x[i-1-j];
      }
      value-=dnorm(x[i],tmp,sigma,1);
    }
    return value;
  }
  arraytype jacobian(arraytype x){
    arraytype y(x.dim);
    y.setZero();
    for(int i=0;i<k;i++)
      for(int j=0;j<k;j++)
	y.col(i)=y.col(i)+Q0(i,j)*x.col(j);
    vectortype v(k+1);
    v(0)=scalartype(1);
    for(int i=1;i<=k;i++)v[i]=-phi[i-1];
    v=v/sigma;
    for(int i=k;i<x.cols();i++){
      for(int j1=0;j1<=k;j1++){
	for(int j2=0;j2<=k;j2++){
	  y.col(i-j1)=y.col(i-j1)+v[j1]*v[j2]*x.col(i-j2);
	}
      }
    }
    return y;
  }
  int ndim(){return 1;}
  VARIANCE_NOT_YET_IMPLEMENTED;
};


/** \brief Continuous AR(2) process

    \verbatim
    Process with covariance satisfying the 2nd order ode 
    rho''=c1*rho'-rho on an arbitrary irregular grid. 
    (shape=c1/2, -1<shape<1). Initial condition rho(0)=1, rho'(0)=0,
    rho''(0)=-1.
    \endverbatim
    
    Process is augmented with derivatives in order to obtain exact sparseness
    of the full precision. That is, if a model is desired on a grid of size n,
    then additional n extra nuisance parameters must be supplied.
    
    @param grid_ Possibly irregular grid of length n
    @param shape_ Parameter defining the shape of the correlation function.
    @param scale_ Parameter defining the correlation range.  
*/
template <class scalartype_>
class contAR2_t{
  TYPEDEFS(scalartype_);
private:
  typedef Matrix<scalartype,2,2> matrix2x2;
  typedef Matrix<scalartype,2,1> matrix2x1;
  typedef Matrix<scalartype,4,4> matrix4x4;
  typedef Matrix<scalartype,4,1> matrix4x1;
  scalartype shape,scale,c0,c1;
  vectortype grid;
  matrix2x2 A, V0, I;
  matrix4x4 B, iB; /* B=A %x% I + I %x% A  */
  matexp<scalartype,2> expA;
  matrix4x1 vecSigma,iBvecSigma;
  vector<MVNORM_t<scalartype> > neglogdmvnorm; /* Cache the 2-dim increments */
  vector<matrix2x2 > expAdt; /* Cache matrix exponential for grid increments */
public:
  contAR2_t(){};
  contAR2_t(vectortype grid_, scalartype shape_, scalartype scale_=1){
    shape=shape_;scale=scale_;grid=grid_;
    c0=scalartype(-1);c1=scalartype(2)*shape_;
    c0=c0/(scale*scale); c1=c1/scale;
    A << scalartype(0), scalartype(1), c0, c1;
    V0 << 1,0,0,-c0;
    I.setIdentity();
    B=kronecker(I,A)+kronecker(A,I);
    iB=B.inverse();
    expA=matexp<scalartype,2>(A);
    vecSigma << 0,0,0,scalartype(-2)*c1*V0(1,1);
    iBvecSigma=iB*vecSigma;
    /* cache increment distribution N(0,V(dt)) - one for each grid point */
    neglogdmvnorm.resize(grid.size());
    neglogdmvnorm[0]=MVNORM_t<scalartype>(V0);
    for(int i=1;i<grid.size();i++)neglogdmvnorm[i]=MVNORM_t<scalartype>(V(grid(i)-grid(i-1)));
    /* cache matrix exponential */
    expAdt.resize(grid.size());
    expAdt[0]=expA(scalartype(0));
    for(int i=1;i<grid.size();i++)expAdt[i]=expA(grid(i)-grid(i-1));
  }
  /* Simple formula for matrix exponential exp(B*t) */
  matrix4x4 expB(scalartype t){
    return kronecker(expA(t),expA(t));
  }
  /* Variance as fct. of time when started deterministic */
  matrix2x2 V(scalartype t){
    matrix4x1 tmp;
    tmp=expB(t)*iBvecSigma-iBvecSigma;
    matrix2x2 ans;
    for(int i=0;i<4;i++)ans(i)=tmp(i);
    return ans;
  }
  /** \brief Evaluate the negative log density of the process x with 
      nuisance parameters dx */
  scalartype operator()(vectortype x,vectortype dx){
    matrix2x1 y, y0;
    scalartype ans;
    y0 << x(0), dx(0);
    ans = neglogdmvnorm[0](y0);
    for(int i=1;i<grid.size();i++){
      y0 << x(i-1), dx(i-1);
      y << x(i), dx(i);
      ans += neglogdmvnorm[i](y-expAdt[i]*y0);
    }
    return ans;
  }
  /* Experiment: Implementing matrix-vector multiply - Q*x.
     To be used when creating separable extensions... 
     think best to assume that input array has dimension (2,ntime,...) 
     so that we can easily extract x(t) and multiply Q*x(t) with a 2x2 matrix */
  scalartype operator()(vectortype x){ /* x.dim=[2,n] */
    vector<int> dim(2);
    dim << 2 , x.size()/2 ;
    array<scalartype> y(x,dim);
    y=y.transpose();
    return this->operator()(y.col(0),y.col(1));
  }
  arraytype matmult(matrix2x2 Q,arraytype x){
    arraytype y(x.dim);
    y.col(0) = Q(0,0)*x.col(0)+Q(0,1)*x.col(1); /* TODO: can we subassign like this in array class? Hack: we use "y.row" for that */
    y.col(1) = Q(1,0)*x.col(0)+Q(1,1)*x.col(1);
    return y;
  }
  arraytype jacobian(arraytype x){
    arraytype y(x.dim);
    y.setZero();
    arraytype tmp(y(0).dim);
    y.col(0) = neglogdmvnorm[0].jacobian(x.col(0)); /* Time zero contrib */
    for(int i=1;i<grid.size();i++){
      /* When taking derivative of .5*(x(i)-G*x(i-1))'*Q*(x(i)-G*x(i-1)) [where G=expAdt]
	 we get contributions like: 
	 x(i-1): -G'*Q*(x(i)-G*x(i-1))
	 x(i):   Q*(x(i)-G*x(i-1))
      */
      tmp=neglogdmvnorm[i].jacobian( x.col(i) - matmult(expAdt[i],x.col(i-1)) );
      y.col(i)=y.col(i)+tmp;
      y.col(i-1)=y.col(i-1)-matmult(expAdt[i].transpose(), tmp );
    }
    return y;
  } 
  int ndim(){return 2;} /* Number of dimensions this structure occupies in total array */
  VARIANCE_NOT_YET_IMPLEMENTED;
};
template <class scalartype, class vectortype>
contAR2_t<scalartype> contAR2(vectortype grid_, scalartype shape_, scalartype scale_=1){
  return contAR2_t<scalartype>(grid_, shape_, scale_);
}
template <class scalartype>
contAR2_t<scalartype> contAR2(scalartype shape_, scalartype scale_=1){
  return contAR2_t<scalartype>(shape_, scale_);
}

/** \brief GMRF on d-dimensional lattice.

    \verbatim
    First order Gaussian Markov Random Field on (subset of) d-dim grid.
    Grid is specified through the first array argument to constructor, 
    with individual nodes determined by the outdermost dimension 
    e.g. x= 1 1 2 2
         1 2 1 2
    corresponding to a 2x2 lattice with 4 nodes and d=2.

    Example of precision in 2D:

       -1
    -1 4+c -1
       -1

   The precision Q is convolved with it self "order" times. This way
   more smoothness can be obtained. The quadratic form contribution 
   is .5*x'*Q^order*x

   The parameter "delta" describes the (inverse) correlation. It is
   allowed to specify a vector of deltas so that different spatial 
   regions can have different spatial correlation.
   
   NOTE: The variance in the model depends on delta. In other words:
   The model may be thought of as an arbitrary scaled correlation 
   model and is thus not really meaningful without an additional scale
   parameter (see SCALE_t and VECSCALE_t classes).
   \endverbatim
*/
//template <class scalartype, class vectortype, class arraytype>
template <class scalartype_>
class GMRF_t{
  TYPEDEFS(scalartype_);
private:
  std::vector<int> row;
  std::vector<int> col;
  int nnz;
  matrix<scalartype> Q;
  scalartype logdetQ;
  int order;
  int sqdist(vectortype x, vectortype x_){
    int ans=0;
    int tmp;
    for(int i=0;i<x.size();i++){
      tmp=CppAD::Integer(x[i])-CppAD::Integer(x_[i]);
      ans+=tmp*tmp;
    }
    return ans;
  }
public:
  GMRF_t(){};
  GMRF_t(arraytype x, vectortype delta, int order_=1){
    int n=x.cols();
    Q.resize(n,n);
    for(int i=0;i<n;i++)
      for(int j=0;j<n;j++){
	if(sqdist(x(i),x(j))==1){
	  Q(i,j)=scalartype(-1);
	  Q(i,i)+=scalartype(1);
	  row.push_back(i);
	  col.push_back(j);
	}
      }
    for(int i=0;i<n;i++){Q(i,i)+=delta[i];row.push_back(i);col.push_back(i);}
    matrix<scalartype> L=Q.llt().matrixL(); /*L L' = Q*/
    logdetQ=scalartype(0);
    for(int i=0;i<n;i++)logdetQ += scalartype(2)*log(L(i,i));
    nnz=row.size();
    order=order_;
    /* logdet(Q^order) */
    logdetQ=scalartype(order)*logdetQ;
  }
  /* Quadratic form: x'*Q^order*x */
  scalartype Quadform(vectortype x){
    scalartype ans=0;
    vectortype y(x.size());
    vectortype z(x);
    for(int k=0;k<order;k++){
      y=z;
      for(int i=0;i<z.size();i++)z[i]=scalartype(0);
      for(int i=0;i<nnz;i++){
	z[row[i]]+=Q(row[i],col[i])*y[col[i]];
      }
    }
    for(int i=0;i<z.size();i++)ans+=x[i]*z[i];
    return ans;
  }
  scalartype operator()(vectortype x){
    return -scalartype(.5)*logdetQ + scalartype(.5)*Quadform(x);
  }
  /* WHY THIS METHOD? :In the array case we let the outer dim be gmrf and inner dim iid N01 */
  // scalartype operator()(arraytype x){    
  //   vector<int> newdim(2);
  //   newdim << x.size()/x.dim[0], x.dim[0];
  //   arraytype y=x.transpose(); /* dim becomes e.g. (dim[2],dim[1],dim[0])  */
  //   y.setdim(newdim);          /* dim becomes e.g. (dim[2]*dim[1],dim[0])  */
  //   scalartype ans=0;
  //   for(int i=0;i<y.rows();i++){
  //     ans+=-scalartype(.5)*logdetQ + scalartype(.5)*Quadform(y(i)); /* copied from previous */
  //   }
  //   return ans;
  // }

  /* jacobian along the lines of Quadform */
  arraytype jacobian(arraytype x){
    arraytype y(x);
    arraytype z(x);
    for(int k=0;k<order;k++){
      y=z;
      z.setZero();
      //for(int i=0;i<z.size();i++)z[i]=scalartype(0);
      for(int i=0;i<nnz;i++){
	//z[row[i]]+=Q(row[i],col[i])*y[col[i]];
	z.col(row[i]) = z.col(row[i]) + Q(row[i],col[i])*y.col(col[i]);
      }
    }
    return z;
  }
  int ndim(){return 1;}
  vectortype variance(){
    int n=Q.rows();
    vectortype ans(n);
    matrixtype C=invertSparseMatrix(Q);
    for(int i=0;i<n;i++)ans[i]=C(i,i);
    return ans;
  }
};
template <class scalartype, class arraytype >
GMRF_t<scalartype> GMRF(arraytype x, vector<scalartype> delta, int order=1){
  return GMRF_t<scalartype>(x, delta, order);
}
template <class scalartype, class arraytype >
GMRF_t<scalartype> GMRF(arraytype x, scalartype delta, int order=1){
  vector<scalartype> d(x.rows());
  for(int i=0;i<d.size();i++)d[i]=delta;
  return GMRF_t<scalartype>(x, d, order);
}

/** \brief Apply scale transformation on a density

    Assume x has density f. Construct the density of y=scale*x where scale is a scalar.

    @param f_ distribution
    @param scale_ scalar
*/ 
template <class distribution>
class SCALE_t{
  TYPEDEFS(typename distribution::scalartype);
private:
  distribution f;
  scalartype scale;
public:
  SCALE_t(){}
  SCALE_t(distribution f_, scalartype scale_){scale=scale_;f=f_;}
  /** \brief Evaluate the negative log density */
  scalartype operator()(arraytype x){
    scalartype ans=f(x/scale);
    ans+=x.size()*log(scale);
    return ans;
  }
  arraytype jacobian(arraytype x){
    return f.jacobian(x/scale)/scale;    
  }
  int ndim(){return f.ndim();}
  vectortype variance(){
    return (scale*scale)*f.variance();
  }
};
template <class scalartype, class distribution>
SCALE_t<distribution> SCALE(distribution f_, scalartype scale_){
  return SCALE_t<distribution>(f_,scale_);
}

/** \brief Apply a vector scale transformation on a density

    Assume x has density f. Construct the density of y=scale*x where scale is a vector.

    @param f_ distribution
    @param scale_ vector
*/ 
template <class distribution>
class VECSCALE_t{
  TYPEDEFS(typename distribution::scalartype);
private:
  distribution f;
  vectortype scale;
public:
  VECSCALE_t(){}
  VECSCALE_t(distribution f_, vectortype scale_){scale=scale_;f=f_;}
  /** \brief Evaluate the negative log density */
  scalartype operator()(arraytype x){
    // assert that x.size()==scale.size()
    scalartype ans=f(x/scale);
    ans+=(log(scale)).sum();
    return ans;
  }
  arraytype jacobian(arraytype x){
    // assert that x.rows()==scale.size()
    arraytype y(x);
    for(int i=0;i<y.cols();i++)y.col(i)=y.col(i)/scale[i];
    y=f.jacobian(y);
    for(int i=0;i<y.cols();i++)y.col(i)=y.col(i)/scale[i];
    return y;
  }
  int ndim(){return f.ndim();}
  VARIANCE_NOT_YET_IMPLEMENTED;
};
template <class vectortype, class distribution>
VECSCALE_t<distribution> VECSCALE(distribution f_, vectortype scale_){
  return VECSCALE_t<distribution>(f_,scale_);
}


/** \brief Separable extension of two densitites

    Take two densities and construct the density of their separable
    extension, defined as the multivariate Gaussian distribution
    with covariance matrix equal to the kronecker product between
    the covariance matrices of the two distributions.

    \verbatim
    More precisely: evaluate density 
    h(x)=|S/(2*pi)|^.5*exp(-.5*x'*S*x) 
    where S=kronecker(Q,R)=Q%x%R assuming we have access to densities
    f(x)=|Q/(2*pi)|^.5*exp(-.5*x'*Q*x)
    g(x)=|R/(2*pi)|^.5*exp(-.5*x'*R*x)
    Let nq=nrow(Q) and nr=nrow(R),
    using rules of the kronecker product we have that
    * Quadratic form = .5*x'*S*x = .5*x'*(Q%x%I)*(I%x%R)*x 
    * Normalizing constant = 
    |S/(2*pi)|^.5 = 
    |(Q/sqrt(2*pi))%x%(R/sqrt(2*pi))|^.5 =
    |(Q/sqrt(2*pi))|^(nr*.5) |(R/sqrt(2*pi))|^(nq*.5) =
    ... something that can be expressed through the normalizing
    constants f(0) and g(0) ...
    f(0)^nr * g(0)^nq * sqrt(2*pi)^(nq*nr)
    \endverbatim

    Example:
    \code
    // Separable extension of two AR1 processes
    Type phi1=0.8;
    AR1_t<N01<Type> > f(phi1);
    Type phi2=0.8;
    AR1_t<N01<Type> > g(phi2);
    SEPARABLE_t<AR1_t<N01<Type> > , AR1_t<N01<Type> > > h(f,g);
    // Can be evaluated on an array:
    array<Type> x(10,20);
    Type ans=h(x);
    \endcode

*/ 
//template <class scalartype, class vectortype, class arraytype, class distribution1, class distribution2>
template <class distribution1, class distribution2>
class SEPARABLE_t{
  TYPEDEFS(typename distribution1::scalartype);
private:
  distribution1 f;
  distribution2 g;
public:
  SEPARABLE_t(){}
  SEPARABLE_t(distribution1 f_, distribution2 g_){f=f_;g=g_;}
  arraytype jacobian(arraytype x){
    int n=f.ndim();
    x=f.jacobian(x);
    x=x.rotate(-n);
    x=g.jacobian(x);
    x=x.rotate(n);
    return x;
  }
  /* Create zero vector corresponding to the first n dimensions of dimension-vector d */
  arraytype zeroVector(vector<int> d, int n){
    int m=1;
    vector<int> newdim(n);
    for(int i=0;i<n;i++){m=m*d[i];newdim[i]=d[i];}
    vectortype x(m);
    x.setZero();
    return arraytype(x,newdim);
  }
  scalartype operator()(arraytype x){
    if(this->ndim() != x.dim.size())std::cout << "Wrong dimension in SEPARABLE_t\n";
    /* Calculate quadform */
    arraytype y(x.dim);
    y=jacobian(x);
    y=x*y; /* pointwise */
    scalartype q=scalartype(.5)*(y.sum()); 
    /* Add normalizing constant */
    int n=f.ndim();
    arraytype zf=zeroVector(x.dim,n);
    q+=f(zf)*(scalartype(x.size())/scalartype(zf.size()));
    x=x.rotate(-n);
    int m=g.ndim();
    arraytype zg=zeroVector(x.dim,m);
    q+=g(zg)*(scalartype(x.size())/scalartype(zg.size()));
    q-=log(sqrt(2.0*M_PI))*(zf.size()*zg.size());
    /* done */
    return q;
  }
  int ndim(){return f.ndim()+g.ndim();}
  VARIANCE_NOT_YET_IMPLEMENTED;

  /* For parallel accumulation:
     ==========================
     Copied operator() above and added extra argument "i" to divide the accumulation
     in chunks. The evaluation of
         operator()(x)
     is equivalent to summing up
         operator()(x,i)
     with i running through the _outer_dimension_ of x.
  */
  scalartype operator()(arraytype x, int i){
    if(this->ndim() != x.dim.size())std::cout << "Wrong dimension in SEPARABLE_t\n";
    /* Calculate quadform */
    arraytype y(x.dim);
    y=jacobian(x);
    y=x*y; /* pointwise */
    scalartype q=scalartype(.5)*(y.col(i).sum()); 
    /* Add normalizing constant */
    if(i==0){
      int n=f.ndim();
      arraytype zf=zeroVector(x.dim,n);
      q+=f(zf)*(scalartype(x.size())/scalartype(zf.size()));
      x=x.rotate(-n);
      int m=g.ndim();
      arraytype zg=zeroVector(x.dim,m);
      q+=g(zg)*(scalartype(x.size())/scalartype(zg.size()));
      q-=log(sqrt(2.0*M_PI))*(zf.size()*zg.size());
    }
    /* done */
    return q;
  }


};

template <class distribution1, class distribution2>
SEPARABLE_t<distribution1,distribution2> SEPARABLE(distribution1 f_, distribution2 g_){
  return SEPARABLE_t<distribution1,distribution2>(f_,g_);
}

/** \brief Projection of multivariate gaussian variable.

    Preserves sparseness if possible. Generally it is not.

    \verbatim
    Given a gaussian density f:R^n -> R.
    Given an integer vector "proj" with elements in 1,...,n.
    Construct the mariginal density of "x[proj]".
   
    Details:
    --------
    Let x=[x_A]
          [x_B]
    with precision
        Q=[Q_AA  Q_AB]
          [Q_BA  Q_BB]
    and assume that proj=A.
    The marginal density is (with notation 0:=0*x_B )
    p_A(x_A)=p(x_A,x_B)/p(x_B|x_A)=p(x_A,0)/p(0|x_A)
    Now see that
    1. p(x_A,0) is easy because full precision is sparse.
    2. p(0|x_A) is N(-Q_BB^-1 * Q_BA x_A,  Q_BB^-1) so
       p(0|x_A) = |Q_BB|^.5 * exp(-.5*x_A Q_AB * Q_BB^-1 * Q_BA x_A)

       Trick to evaluate this with what we have available:
       Note 1: Q_BA x_A = [0 I_BB] * full_jacobian([ x_A  ]  
                                                   [ 0    ] )


	       Call this quantity "y_B" we have
	       p(0|x_A) = |Q_BB|^.5 * exp(-.5*y_B' * Q_BB^-1 * y_B)

       Note 2: Consider now a density with _covariance_ Q_BB 
               phi(y)=|Q_BB|^-.5 * exp(-.5*y' * Q_BB^-1 * y)
	       Then 
	       phi(y)/phi(0)^2=|Q_BB|^.5 * exp(-.5*y' * Q_BB^-1 * y)
	       which is actually the desired expression of p(0|x_A).

    Summary:
    -------
    Negative log-density of A-marginal is
    -log p(x_A,0) + log phi(y) - 2*log(phi(0))
    = f(x_A,0) - dmvnorm(y_B) + 2*dmvnorm(0)
    \endverbatim
*/
template <class distribution>
class PROJ_t{
  TYPEDEFS(typename distribution::scalartype);
private:
  distribution f;
  bool initialized;
public:
  vector<int> proj;
  vector<int> cproj; /* complementary proj _sorted_ */
  int n,nA,nB;
  matrixtype Q;  /* Full precision */
  MVNORM_t<scalartype> dmvnorm; /* mean zero gaussian with covariance Q_BB */
  PROJ_t(){}
  PROJ_t(distribution f_, vector<int> proj_){
    f=f_;
    proj=proj_;
    initialized=false;
  }
  void initialize(int n_){
    if(!initialized){
      n=n_;
      nA=proj.size();
      nB=n-nA;
      cproj.resize(nB);
      vector<int> mark(n);
      mark.setZero();
      for(int i=0;i<nA;i++)mark[proj[i]]=1;
      int k=0;
      for(int i=0;i<n;i++)if(!mark[i])cproj[k++]=i;
      // Full precision
      //matrixtype I(n,n);
      //I.setIdentity();
      //vectortype v(I);

      k=0;
      vectortype v(n*n);
      for(int i=0;i<n;i++)
	for(int j=0;j<n;j++)
	  v(k++)=scalartype(i==j);

      vector<int> dim(2);
      dim << n,n;
      arraytype a(v,dim);
      a=f.jacobian(a);
      Q.resize(n,n);
      for(int i=0;i<n*n;i++)Q(i)=a[i];
      // Get Q_BB
      matrixtype QBB(nB,nB);
      for(int i=0;i<nB;i++)
	for(int j=0;j<nB;j++)
	  QBB(i,j)=Q(cproj[i],cproj[j]);
      dmvnorm=MVNORM_t<scalartype>(QBB);
    }
    initialized=true;
  }
  vectortype projB(vectortype x){
    vectortype y(nB);
    for(int i=0;i<nB;i++)y[i]=x[cproj[i]];
    return y;
  }
  vectortype setZeroB(vectortype x){
    for(int i=0;i<nB;i++)x[cproj[i]]=scalartype(0);
    return x;    
  }
  scalartype operator()(vectortype x){
    initialize(x.size());
    x=setZeroB(x);
    vector<int> dim(1);
    dim << x.size();
    arraytype xa(x,dim);
    vectortype y=projB(f.jacobian(xa));
    // f(x_A,0) - dmvnorm(y_B) + 2*dmvnorm(0)
    return f(xa) - dmvnorm(y) + 2*dmvnorm(y*scalartype(0));
  }
  /* array versions  */
  arraytype projB(arraytype x){
    vectortype z((x.size()/n)*nB);
    vector<int> dim(x.dim);
    dim[dim.size()-1]=nB;
    arraytype y(z,dim);
    for(int i=0;i<nB;i++)y.col(i)=x.col(cproj[i]);
    return y;
  }
  arraytype setZeroB(arraytype x){
    for(int i=0;i<nB;i++)x.col(cproj[i])=x.col(0)*scalartype(0);
    return x;    
  }
  arraytype jacobian(arraytype x){
    initialize(x.dim[x.dim.size()-1]);
    arraytype xa=setZeroB(x);
    arraytype y=projB(f.jacobian(xa));
    // WRONG: ----> return f.jacobian(xa) - dmvnorm.jacobian(y);
    // y=P*Q*Z*x  so should be  (P*Q*Z)' * dmvnorm.jacobian(y).
    // Note: only P is not symmetric.
    arraytype tmp=f.jacobian(xa);
    arraytype tmp0=tmp*scalartype(0);
    arraytype tmp2=dmvnorm.jacobian(y);
    // apply P'
    for(int i=0;i<nB;i++){ 
      tmp0.col(cproj[i])=tmp2.col(i);
    }
    // apply Q'(=Q)
    tmp0=f.jacobian(tmp0);
    // apply Z'(=Z)
    tmp0=setZeroB(tmp0);
    // Done:
    return tmp-tmp0;
  }
  int ndim(){return f.ndim();}
  VARIANCE_NOT_YET_IMPLEMENTED;
};

template <class distribution>
PROJ_t<distribution> PROJ(distribution f_, vector<int> i){
  return PROJ_t<distribution>(f_,i);
}


#undef TYPEDEFS