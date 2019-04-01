const express = require('express');
const appRoot = require('app-root-path');

const app = express();

const PORT = 8080;


app.use(express.static(`${appRoot}/dist`));

app.listen(PORT,()=>console.log(`Listening on port ${PORT}`));
