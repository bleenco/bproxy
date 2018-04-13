import * as chai from 'chai';
import * as chaiAsPromised from 'chai-as-promised';

chai.use(chaiAsPromised);
chai.use(require('chai-http'));

const expect = chai.expect;

describe('Check 404 responses', () => {

  it(`should return 404 response based on http://localhost:8080 request`, () => {
    return chai
      .request('http://localhost:8080')
      .get('/')
      .then(resp => {
        expect(resp).to.have.status(404);
        expect(resp).to.be.html;
        expect(resp).to.not.redirect;
      });
  });

});
