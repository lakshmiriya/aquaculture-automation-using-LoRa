function complicated(byte){
  var result = parseInt(byte);
  var reminder = result;
  var org = "";
  if (reminder > 16)
  {
     while (reminder > 16){
    org += String(parseInt(reminder/16)) + String(parseInt(reminder%16));
    reminder = reminder % 16;
  }
    
  }
  else{
    if (reminder<10)org = "0"+String(reminder);
    else if(reminder==16)org="10";
    else{org = String(reminder)}
  }
  
  return org;
}

function Decoder(bytes, port) {
  var value = []; 
  var index = 0;
  var temp = "";
  for (var i=0; i<bytes.length;i++)
  {
    
    if (parseInt(bytes[i]) === 32)
    {
      value.push(temp);
      temp = "";
    }
    else
    {
      temp += complicated(bytes[i]); 
    }
  }
return {
   pH:value[0]/100,
   alk:value[1]/100,
   temp:value[2]/100,
   tds:value[3]/1
};
}
