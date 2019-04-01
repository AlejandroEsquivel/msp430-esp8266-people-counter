const express = require('express');
const appRoot = require('app-root-path');

const app = express();

const PORT = 8080;

const dir = `${appRoot}/dist`;

app.use(express.static(dir));

app.get('/',(req,res)=>res.status(200).send('ok'));

app.listen(PORT,()=>console.log(`Listening on port ${PORT}, serving ${dir}...`));
