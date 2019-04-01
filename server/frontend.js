const express = require('express');

const app = express();

const PORT = 8080;

app.use(express.static('../dist'));

app.get('/',(req,res)=>res.status(200).send('ok'));

app.listen(PORT,()=>console.log(`Listening on port ${PORT}...`));
