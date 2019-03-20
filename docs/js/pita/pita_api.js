var pitaCaller;

var pitaModule = Module().then(function(instance){
  pitaCaller = {
    inst: instance,
    run: function(str) {
      var buffer = new Uint8Array(str.length);
      for( var i = 0; i < str.length; i++ ) {
        buffer[i] = str.charCodeAt(i);
      }

      //console.log(instance._malloc);

      var pointer = instance._malloc(buffer.length);
      var offset = pointer;
      instance.HEAPU8.set(buffer, offset);

      //console.log(str);
      //console.log(buffer);

      var result = instance.__Z11executePitaPcj(pointer, buffer.length);
      instance._free(pointer);

      var offset = instance.__Z16getPitaResultPtrv();
      var length = instance.__Z19getPitaResultLengthv();
      
      //console.log(instance.HEAPU8[offset]);
      var buffer =  new Uint8Array(length);
      for(var i = 0; i < length; i++)
      {
        buffer[i] = instance.HEAPU8[offset + i];
      }

      var result = String.fromCharCode.apply(null, buffer);
      //console.log(result);
      return result;
    },
    test: function(str) {
      console.log(str);
    }
  };
/*
  console.log(inst);

  function pita(str) {
    var buffer = new Uint8Array(str.length);
    for( var i = 0; i < str.length; i++ ) {
      buffer[i] = str.charCodeAt(i);
    }
    //buffer[str.length] = 0;

    console.log(instance._malloc);

    var pointer = instance._malloc(buffer.length);
    var offset = pointer;
    instance.HEAPU8.set(buffer, offset);

    console.log(str);
    console.log(buffer);

    var result = instance.__Z11executePitaPcj(pointer, buffer.length);
    instance._free(pointer);
  }

  console.log("test2");

  var txt = (function(param) {return param[0].replace(/\n|\r/g, "");})`
  main = Shape{
    a: Square{scale = {x: 30, y: 30}, fill: {r: 255, g: 0, b: 0}},
    b: Square{scale = {x: 40, y: 40}, fill: {r: 0, g: 255, b: 0}},
    c: Square{scale = {x: 50, y: 50}, fill: {r: 0, g: 0, b: 255}},

    sat( Contact(a.topRight(), b.bottomLeft()) & Contact(a.bottomRight(), c.topLeft()) ),
    sat( Contact(b.bottomRight(), c.topRight()) ),
    var(b.pos in [-200, 200], c.pos in [-200, 200], c.angle in [0, 90])
  }`;

  console.log(txt);

  pita(txt);

  var offset = instance.__Z16getPitaResultPtrv();
  var length = instance.__Z19getPitaResultLengthv();
  
  console.log(instance.HEAPU8[offset]);
  var buffer =  new Uint8Array(length);
  for(var i = 0; i < length; i++)
  {
    buffer[i] = instance.HEAPU8[offset + i];
  }
  console.log( String.fromCharCode.apply(null, buffer) );
  */
});

/*
setTimeout(() => {
  //console.log(pitaCaller.run("Square{}"));
  console.log(pitaCaller.run("Square{}"));
  //console.log(pitaCaller);
}, 2000);
*/
