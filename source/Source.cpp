#define NOMINMAX

//#define CGL_EnableLogOutput
//#define CGL_DO_TEST

#include <iostream>
#include <fstream>

#include <Pita/Program.hpp>

extern bool calculating;

#ifdef CGL_DO_TEST
int main()
{
	ofs.open("log.txt");

	using namespace cgl;

	const auto parse = [](const std::wstring& str, Lines& lines)->bool
	{
		using namespace cgl;

		SpaceSkipper<IteratorT> skipper;
		Parser<IteratorT, SpaceSkipperT> grammer;

		std::wstring::const_iterator it = str.begin();
		if (!boost::spirit::qi::phrase_parse(it, str.end(), grammer, skipper, lines))
		{
			std::cerr << "error: parse failed\n";
			return false;
		}

		if (it != str.end())
		{
			std::cerr << "error: ramains input\n" << std::wstring(it, str.end());
			return false;
		}

		return true;
	};

#ifdef DO_TEST

	std::vector<std::wstring> test_ok({
		"(1*2 + -3*-(4 + 5/6))",
		"1/(3+4)^6^7",
		"1 + 2, 3 + 4",
		"\n 4*5",
		"1 + 1 \n 2 + 3",
		"1 + 2 \n 3 + 4 \n",
		"1 + 1, \n 2 + 3",
		"1 + 1 \n \n \n 2 + 3",
		"1 + \n \n \n 2",
		"1 + 2 \n 3 + 4 \n\n\n\n\n",
		"1 + 2 \n , 4*5",
		"1 + 3 * \n 4 + 5",
		"(-> 1 + 2)",
		"(-> 1 + 2 \n 3)",
		"x, y -> x + y",
		"fun = (a, b, c -> d, e, f -> g, h, i -> a = processA(b, c), d = processB((a, e), f), g = processC(h, (a, d, i)))",
		"fun = a, b, c -> d, e, f -> g, h, i -> a = processA(b, c), d = processB((a, e), f), g = processC(h, (a, d, i))",
		"gcd1 = (m, n ->\n if m == 0\n then return m\n else self(mod(m, n), m)\n)",
		"gcd2 = (m, n ->\r\n if m == 0\r\n then return m\r\n else self(mod(m, n), m)\r\n)",
		"gcd3 = (m, n ->\n\tif m == 0\n\tthen return m\n\telse self(mod(m, n), m)\n)",
		"gcd4 = (m, n ->\r\n\tif m == 0\r\n\tthen return m\r\n\telse self(mod(m, n), m)\r\n)",
		R"(
gcd5 = (m, n ->
	if m == 0
	then return m
	else self(mod(m, n), m)
)
)",
R"(
func = x ->  
    x + 1
    return x

func2 = x, y -> x + y)",
"a = {a: 1, b: [1, {a: 2, b: 4}, 3]}, a.b[1].a = {x: 3, y: 5}, a",
"x = 10, f = ->x + 1, f()",
"x = 10, g = (f = ->x + 1, f), g()"
	});
	
	std::vector<std::wstring> test_ng({
		", 3*4",
		"1 + 1 , , 2 + 3",
		"1 + 2, 3 + 4,",
		"1 + 2, \n , 3 + 4",
		"1 + 3 * , 4 + 5",
		"(->)"
	});

	int ok_wrongs = 0;
	int ng_wrongs = 0;

	std::cout << "==================== Test Case OK ====================" << std::endl;
	for (size_t i = 0; i < test_ok.size(); ++i)
	{
		std::cout << "Case[" << i << "]\n\n";

		std::cout << "input:\n";
		std::cout << test_ok[i] << "\n\n";

		std::cout << "parse:\n";

		Lines expr;
		const bool succeed = parse(test_ok[i], expr);

		printExpr(expr);

		std::cout << "\n";

		if (succeed)
		{
			/*
			std::cout << "eval:\n";
			printEvaluated(evalExpr(expr));
			std::cout << "\n";
			*/
		}
		else
		{
			std::cout << "[Wrong]\n";
			++ok_wrongs;
		}

		std::cout << "-------------------------------------" << std::endl;
	}

	std::cout << "==================== Test Case NG ====================" << std::endl;
	for (size_t i = 0; i < test_ng.size(); ++i)
	{
		std::cout << "Case[" << i << "]\n\n";

		std::cout << "input:\n";
		std::cout << test_ng[i] << "\n\n";

		std::cout << "parse:\n";

		Lines expr;
		const bool failed = !parse(test_ng[i], expr);

		printExpr(expr);

		std::cout << "\n";

		if (failed)
		{
			std::cout << "no result\n";
		}
		else
		{
			//std::cout << "eval:\n";
			//printEvaluated(evalExpr(expr));
			//std::cout << "\n";
			std::cout << "[Wrong]\n";
			++ng_wrongs;
		}

		std::cout << "-------------------------------------" << std::endl;
	}

	std::cout << "Result:\n";
	std::cout << "Correct programs: (Wrong / All) = (" << ok_wrongs << " / " << test_ok.size() << ")\n";
	std::cout << "Wrong   programs: (Wrong / All) = (" << ng_wrongs << " / " << test_ng.size() << ")\n";

#endif

#ifdef DO_TEST2

	int eval_wrongs = 0;

	const auto testEval1 = [&](const std::wstring& source, std::function<bool(const Evaluated&)> pred)
	{
		std::cout << "----------------------------------------------------------\n";
		std::cout << "input:\n";
		std::cout << source << "\n\n";

		std::cout << "parse:\n";

		Lines lines;
		const bool succeed = parse(source, lines);

		if (succeed)
		{
			Expr expr = lines;
			printExpr(expr);

			std::cout << "eval:\n";
			Evaluated result = evalExpr(lines);

			std::cout << "result:\n";
			printEvaluated(result, nullptr);

			const bool isCorrect = pred(result);

			std::cout << "test: ";

			if (isCorrect)
			{
				std::cout << "Correct\n";
			}
			else
			{
				std::cout << "Wrong\n";
				++eval_wrongs;
			}
		}
		else
		{
			std::cerr << "Parse error!!\n";
			++eval_wrongs;
		}
	};

	const auto testEval = [&](const std::wstring& source, const Evaluated& answer)
	{
		testEval1(source, [&](const Evaluated& result) {return IsEqualEvaluated(result, answer); });
	};

testEval(R"(

{a: 3}.a

)", 3);

testEval(R"(

f = (x -> {a: x+10})
f(3).a

)", 13);

/*
testEval(R"(

vec3 = (v -> {
	x:v, y : v, z : v
})
vec3(3)

)", Record("x", 3).append("y", 3).append("z", 3));

using Li = cgl::ListConstractor;
Program program;

program.test(R"(

vec2 = (v -> [
	v, v
])
a = vec2(3)
vec2(a)

)", Li(Li({ 3, 3 }))(Li({ 3, 3 })));


program.test(R"(

vec2 = (v -> [
	v, v
])
vec2(vec2(3))

)", Li(Li({ 3, 3 }))(Li({ 3, 3 })));
*/


/*
testEval(R"(

vec2 = (v -> [
	v, v
])
a = vec2(3)
vec2(a)

)", List().append(List().append(3).append(3)).append(List().append(3).append(3)));
*/
/*
testEval(R"(

vec2 = (v -> [
	v, v
])
vec2(vec2(3))

)", List().append(List().append(3).append(3)).append(List().append(3).append(3)));
*/

/*
testEval(R"(

vec2 = (v -> {
	x:v, y : v
})
a = vec2(3)
vec2(a)

)", Record("x", Record("x", 3).append("y", 3)).append("y", Record("x", 3).append("y", 3)));

testEval(R"(

vec2 = (v -> {
	x:v, y : v
})
vec2(vec2(3))

)", Record("x", Record("x", 3).append("y", 3)).append("y", Record("x", 3).append("y", 3)));


//このプログラムについて、LINES_Aが出力されてLINES_Bが出力される前に落ちるバグ有り
testEval(R"(

vec3 = (v -> {
	x:v, y : v, z : v
})
mul = (v1, v2 -> {
	x:v1.x*v2.x, y : v1.y*v2.y, z : v1.z*v2.z
})
mul(vec3(3), vec3(2))

)", Record("x", 6).append("y", 6).append("z", 6));

testEval(R"(

r = {x: 0, y:10, sat(x == y), free(x)}
r.x

)", 10.0);

testEval(R"(

a = [1, 2]
b = [a, 3]

)", List().append(List().append(1).append(2)).append(3));

testEval(R"(

a = {a:1, b:2}
b = {a:a, b:3}

)", Record("a", Record("a", 1).append("b", 2)).append("b", 3));

testEval1(R"(

shape = {
	pos: {x:0, y:0}
	scale: {x:1, y:1}
}

line = shape{
	vertex: [
		{x:0, y:0}
		{x:1, y:0}
	]
}

main = {
	l1: line{
		vertex[1].y = 10
		color: {r:255, g:0, b:0}
	}
	l2: line{
		vertex[0] = {x: 2, y:3}
		color: {r:0, g:255, b:0}
	}

	sat(l1.vertex[1].x == l2.vertex[0].x & l1.vertex[1].y == l2.vertex[0].y)
	free(l1.vertex[1])
}

)", [](const Evaluated& result) {
	const Evaluated l1vertex1 = As<List>(As<Record>(As<Record>(result).values.at("l1")).values.at("vertex"));
	const Evaluated answer = List().append(Record("x", 0).append("y", 0)).append(Record("x", 2).append("y", 3));
	printEvaluated(answer);
	return IsEqual(l1vertex1, answer);
});

testEval1(R"(

main = {
    x: 1
    y: 2
    r: 1
    theta: 0
    sat(r*cos(theta) == x & r*sin(theta) == y)
    free(r, theta)
}

)", [](const Evaluated& result) {
	return IsEqual(As<Record>(result).values.at("theta"),1.1071487177940905030170654601785);
});
*/


/*

rod = {
    r: 10
    verts: [
        {x:0, y:0}
        {x:r, y:0}
    ]
}

newRod = (x, y -> rod{verts:[{x:x, y:y}, {x:x+r, y:y}]})

rod2 = {
    rods: [newRod(0,0),newRod(10,10),newRod(20,20),newRod(30,30)]

    sat(rods[0].verts[1].x == rods[0+1].verts[0].x & rods[0].verts[1].y == rods[0+1].verts[0].y)
    sat(rods[1].verts[1].x == rods[1+1].verts[0].x & rods[1].verts[1].y == rods[1+1].verts[0].y)
    sat(rods[2].verts[1].x == rods[2+1].verts[0].x & rods[2].verts[1].y == rods[2+1].verts[0].y)

    sat((rods[0].verts[0].x - rods[0].verts[1].x)^2 + (rods[0].verts[0].y - rods[0].verts[1].y)^2 == rods[0].r^2)
    sat((rods[1].verts[0].x - rods[1].verts[1].x)^2 + (rods[1].verts[0].y - rods[1].verts[1].y)^2 == rods[1].r^2)
    sat((rods[2].verts[0].x - rods[2].verts[1].x)^2 + (rods[2].verts[0].y - rods[2].verts[1].y)^2 == rods[2].r^2)
    sat((rods[3].verts[0].x - rods[3].verts[1].x)^2 + (rods[3].verts[0].y - rods[3].verts[1].y)^2 == rods[3].r^2)

    sat(rods[0].verts[0].x == 0 & rods[0].verts[0].y == 0)
    sat(rods[3].verts[1].x == 30 & rods[3].verts[1].y == 0)

    free(rods[0].verts, rods[1].verts, rods[2].verts, rods[3].verts)
}

*/

/*
rod = {
    r: 10
    verts: [
        {x:0, y:0}
        {x:r, y:0}
    ]
}

newRod = (x, y -> rod{verts:[{x:x, y:y}, {x:x+r, y:y}]})

rod2 = {
    rods: [newRod(0,0),newRod(10,10),newRod(20,20),newRod(30,30)]

    for i in 0:2 do(
        sat(rods[i].verts[1].x == rods[i+1].verts[0].x & rods[i].verts[1].y == rods[i+1].verts[0].y)
    )

    for i in 0:3 do(
        sat((rods[i].verts[0].x - rods[i].verts[1].x)^2 + (rods[i].verts[0].y - rods[i].verts[1].y)^2 == rods[i].r^2)
    )

    sat(rods[0].verts[0].x == 0 & rods[0].verts[0].y == 0)
    sat(rods[3].verts[1].x == 30 & rods[3].verts[1].y == 0)

    free(rods[0].verts, rods[1].verts, rods[2].verts, rods[3].verts)
}

EOF
*/
	std::cerr<<"Test Wrong Count: " << eval_wrongs<<std::endl;
	
#endif

/*
shape = {
	pos: {x:0, y:0}
	scale: {x:1, y:1}
	angle: 0
}

stick = shape{
	scale = {x:1, y:3}
	vertex: [
		{x: -1, y: -1}, {x: +1, y: -1}
		{x: +1, y: +1}, {x: -1, y: +1}
	]
}

plus = shape{
	scale = {x:50, y:50}
	a: stick{}
	b: stick{angle = 90}
}

cross = shape{
	pos = {x:256, y:256}
	a: plus{angle = 45}
}

EOF
	*/

	
	{
		cgl::Program program;
		program.draw(R"(
shape = {
	pos: {x:0, y:0}
	scale: {x:1, y:1}
	angle: 0
}

stick = shape{
	scale = {x:1, y:3}
	vertex: [
		{x: -1, y: -1}, {x: +1, y: -1}
		{x: +1, y: +1}, {x: -1, y: +1}
	]
}

plus = shape{
	scale = {x:50, y:50}
	a: stick{}
	b: stick{angle = 90}
}

cross = shape{
	pos = {x:256, y:256}
	a: plus{angle = 45}
}
)");
	}

	{
		cgl::Program program;
		program.draw(R"(

shape = {
	pos: {x:0, y:0}
	scale: {x:1, y:1}
	angle: 0
}

transform = (t, p -> {
	cosT:cos(t.angle)
	sinT:sin(t.angle)
	x:t.pos.x + cosT * t.scale.x * p.x - sinT * t.scale.y * p.y
	y:t.pos.y + sinT* t.scale.x* p.x + cosT* t.scale.y*p.y
})

contact = (p, q -> p.x == q.x & p.y == q.y)

square = shape{
	vertex: [
		{x: -1, y: -1}, {x: +1, y: -1}
		{x: +1, y: +1}, {x: -1, y: +1}
	]
}

main = shape{
	a: square{scale: {x:10,y:30}}
	b: square{scale: {x:20,y:10}}
	sat(contact(transform(a, a.vertex[0]), transform(b, b.vertex[2])))
	free(a.pos)
}

)");
	}
	

	{
		cgl::Program program;
		program.draw(R"(

transform = (pos, scale, angle, vertex -> {
	cosT:cos(angle)
	sinT:sin(angle)
	x:pos.x + cosT * scale.x * vertex.x - sinT * scale.y * vertex.y
	y:pos.y + sinT* scale.x * vertex.x + cosT* scale.y * vertex.y
})

shape = {
	pos: {x:0, y:0}
	scale: {x:1, y:1}
	angle: 0
}

square = shape{
	vertex: [
		{x: -1, y: -1}, {x: +1, y: -1}, {x: +1, y: +1}, {x: -1, y: +1}
	]
	topLeft:     (->transform(pos, scale, angle, vertex[0]))
	topRight:    (->transform(pos, scale, angle, vertex[1]))
	bottomRight: (->transform(pos, scale, angle, vertex[2]))
	bottomLeft:  (->transform(pos, scale, angle, vertex[3]))
}

contact = (p, q -> (p.x - q.x)*(p.x - q.x) < 1  & (p.y - q.y)*(p.y - q.y) < 1)

main = shape{

    a: square{scale: {x: 50, y: 50}}
    b: square{scale: {x: 100, y: 50}}
    c: b{}

    sat( contact(a.topRight(), b.bottomLeft()) & contact(a.bottomRight(), c.topLeft()) )
    sat( contact(b.bottomRight(), c.topRight()) )
    free(b.pos, b.angle, c.pos, c.angle)
}
)");
	}
	

	while (true)
	{
		std::wstring source;

		std::wstring buffer;
		while (std::cout << ">> ", std::getline(std::cin, buffer))
		{
			if (buffer == "quit()")
			{
				return 0;
			}
			else if (buffer == "EOF")
			{
				break;
			}

			source.append(buffer + '\n');
		}

		Program program;
		program.draw(source);
		/*Program program;
		try
		{
			program.draw(source);
		}
		catch (const cgl::Exception& e)
		{
			std::cerr << "Exception: " << e.what() << std::endl;
		}*/
	}

	return 0;
}
#else
int main(int argc, char* argv[])
{
	if (argc != 2)
	{
		std::cout << "usage:\n";
		std::cout << "./core source_file\n";
		return 0;
	}

	ofs.open("log.txt");

	const std::string filename(argv[1]);
	std::ifstream ifs(filename);
	std::string sourceCode((std::istreambuf_iterator<char>(ifs)), std::istreambuf_iterator<char>());
	std::cout << sourceCode << std::endl;
	
	calculating = true;
	cgl::Program program;
	//program.draw(sourceCode, false);
	program.execute1(sourceCode);

	return 0;
}
#endif
