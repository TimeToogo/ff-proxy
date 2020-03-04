import { FfClientAgent } from "./src/";
import AWS from "aws-sdk";

// client.sendRequest({
//   request: "GET / HTTP/1.1\nHost: www.google.com\n\n",
//   https: true
// }).then(() => console.log('done'));

console.log("sending request...");

const S3 = new AWS.S3({
  region: "ap-southeast-2",
  httpOptions: {
    agent: new FfClientAgent({
      ipAddress: "127.0.0.1",
      port: 8080,
      https: true,
    })
  }
});

S3.listBuckets()
  .promise()
  .then(res => {
    console.log("the", res.Buckets);
  })
  .catch(e => {
    console.log("cat", e);
  });
