import cppyy

class Compiler:
  def __init__(self):
    pass

def jit(return_type: str, columns: dict={}, expression: str=''):
	'''
	Just-in-time compile a C++ expression

	Parameters
	----------
	expression : string
		a valid C++ expression containing known observables

	Returns
	-------
	lmbd, args
		compiled lambda and variable nodes used as input arguments

	Raises
	------
	RuntimeError
		when compilation fails due to invalid C++ expression and/or unknown variable(s)
	'''
	# 0. generate expression-baed unique names
	func_name = "ana__equation_func"+ROOT.TString(expression).MD5().Data()
	ftor_name = "ana__equation_ftor"+ROOT.TString(expression).MD5().Data()
	lmbd_name = "ana__equation_lmbd"+ROOT.TString(expression).MD5().Data()

	# 1. get columns used in expression
	# existingColumnNames = common.vectorize(observables.keys(),dtype='string')
	# arg_names = ROOT.ana.variable.computation.tokenize_expression(expression,existingColumnNames)
	# args = [observables[arg_name] for arg_name in arg_names]

	# done if lambda is already compiled
	if hasattr(ROOT,lmbd_name):
		lmbd = getattr(ROOT,lmbd_name)
		return lmbd

	# 2. compile function
	arg_decls = []
	for iarg,arg_name in enumerate(arg_names):
		arg_decls.append("{t} {n}".format(t=arg_types[iarg],n=arg_name))

	func_defn = '''
		auto {func} ({pars}) {{ return ({expr}); }}
		'''.format(
			func=func_name,
			pars=', '.join(arg_decls),
			expr=expression
		)
	try:
		cling.Declare(
			func_defn
		)
		func = getattr(ROOT,func_name)
	except Exception as e:
		raise RuntimeError("C++ expression is invalid and/or contains unknown variable(s).".format(expression))

	# 2. compile functor
	ftor_defn = '''
		struct {ftor}
		{{
		auto operator()({args}) {{ return ({func}({names})); }}
		}};
		'''.format(
			ftor=ftor_name,
			args=', '.join(arg_decls),
			func=func_name,
			names=', '.join([str(arg_name) for arg_name in arg_names])
		)
	cling.Declare(ftor_defn)

	# 3. instantiate lambda
	cling.Declare(
		'''{ftor} {lmbd};'''.format(
			ftor=ftor_name,
			lmbd=lmbd_name
		)
	)
	lmbd = getattr(ROOT,lmbd_name)

	# done, return lammda and arguments
	return lmbd
