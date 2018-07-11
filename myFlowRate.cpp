/*---------------------------------------------------------------------------*\
  =========                 |
  \\      /  F ield         | OpenFOAM: The Open Source CFD Toolbox
   \\    /   O peration     |
    \\  /    A nd           | Copyright (C) 2011-2017 OpenFOAM Foundation
     \\/     M anipulation  |
-------------------------------------------------------------------------------
License
    This file is part of OpenFOAM.

    OpenFOAM is free software: you can redistribute it and/or modify it
    under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    OpenFOAM is distributed in the hope that it will be useful, but WITHOUT
    ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
    FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
    for more details.

    You should have received a copy of the GNU General Public License
    along with OpenFOAM.  If not, see <http://www.gnu.org/licenses/>.

\*---------------------------------------------------------------------------*/

#include "myFlowRate.H"
#include "addToRunTimeSelectionTable.H"
#include "volFields.H"
#include "one.H"

//////////////////////this is the space where minegishirei using/////////////////////
#include <fstream>  //for ifstream ,getline
#include <string>   //for string
/////////////////////////////////////////////
// * * * * * * * * * * * * * * * * Constructors  * * * * * * * * * * * * * * //

/*
Foam::myFlowRateFvPatchVectorField::
myFlowRateFvPatchVectorField
(
    const fvPatch& p,
    const DimensionedField<vector, volMesh>& iF
    
)
:
    fixedValueFvPatchField<vector>(p, iF),
    flowRate_(),
    volumetric_(false),
    rhoName_("rho"),
    rhoInlet_(0.0),
    extrapolateProfile_(false)
{
    Info<<"TEST_CONSTRactor1";
}*/


Foam::myFlowRateFvPatchVectorField::
myFlowRateFvPatchVectorField
(
    const fvPatch& p,
    const DimensionedField<vector, volMesh>& iF,
    const dictionary& dict
)
:
    fixedValueFvPatchField<vector>(p, iF, dict, false),
    rhoInlet_(dict.lookupOrDefault<scalar>("rhoInlet", -VGREAT)),
    extrapolateProfile_
    (
        dict.lookupOrDefault<Switch>("extrapolateProfile", false)
    )
{
    Info<<"TEST_CONSTRactor2\n";
    if (dict.found("volumetricFlowRate"))
    {
        volumetric_ = true;
        flowRate_ = Function1<scalar>::New("volumetricFlowRate", dict);
        rhoName_ = "rho";
    }
    else if (dict.found("massFlowRate"))
    {
        volumetric_ = false;
        flowRate_ = Function1<scalar>::New("massFlowRate", dict);
        rhoName_ = word(dict.lookupOrDefault<word>("rho", "rho"));
    }
    else
    {
        FatalIOErrorInFunction
        (
            dict
        )   << "Please supply either 'volumetricFlowRate' or"
            << " 'massFlowRate' and 'rho'" << exit(FatalIOError);
    }

    // Value field require if mass based
    if (dict.found("value"))
    {
        fvPatchField<vector>::operator=
        (
            vectorField("value", dict, p.size())
        );
    }
    else
    {
        evaluate(Pstream::commsTypes::blocking);
    }
}

/*
Foam::myFlowRateFvPatchVectorField::
myFlowRateFvPatchVectorField
(
    const myFlowRateFvPatchVectorField& ptf,
    const fvPatch& p,
    const DimensionedField<vector, volMesh>& iF,
    const fvPatchFieldMapper& mapper
)
:
    fixedValueFvPatchField<vector>(ptf, p, iF, mapper),
    flowRate_(ptf.flowRate_, false),
    volumetric_(ptf.volumetric_),
    rhoName_(ptf.rhoName_),
    rhoInlet_(ptf.rhoInlet_),
    extrapolateProfile_(ptf.extrapolateProfile_)
{
    Info<<"TEST_CONSTRactor3\n";
}


Foam::myFlowRateFvPatchVectorField::
myFlowRateFvPatchVectorField
(
    const myFlowRateFvPatchVectorField& ptf
)
:
    fixedValueFvPatchField<vector>(ptf),
    flowRate_(ptf.flowRate_, false),
    volumetric_(ptf.volumetric_),
    rhoName_(ptf.rhoName_),
    rhoInlet_(ptf.rhoInlet_),
    extrapolateProfile_(ptf.extrapolateProfile_)
{
Info<<"TEST_CONSTRactor4\n";
}
*/

Foam::myFlowRateFvPatchVectorField::
myFlowRateFvPatchVectorField
(
    const myFlowRateFvPatchVectorField& ptf,
    const DimensionedField<vector, volMesh>& iF
)
:
    fixedValueFvPatchField<vector>(ptf, iF),
    flowRate_(ptf.flowRate_, false),
    volumetric_(ptf.volumetric_),
    rhoName_(ptf.rhoName_),
    rhoInlet_(ptf.rhoInlet_),
    extrapolateProfile_(ptf.extrapolateProfile_)
{
Info<<"TEST_CONSTRactor5\n";

}



// * * * * * * * * * * * * * * * Member Functions  * * * * * * * * * * * * * //

template<class RhoType>
void Foam::myFlowRateFvPatchVectorField::updateValues
(
    const RhoType& rho
)
{
    Info<<"TEST_3_UPDATE\n";
    const scalar t = db().time().timeOutputValue();

    const vectorField n(patch().nf());

    if (extrapolateProfile_)
    {
        vectorField Up(this->patchInternalField());

        // Patch normal extrapolated velocity
        scalarField nUp(n & Up);

        // Remove the normal component of the extrapolate patch velocity
        Up -= nUp*n;

        // Remove any reverse flow
        nUp = min(nUp, scalar(0));

        const scalar flowRate = flowRate_->value(t);
        const scalar estimatedFlowRate = -gSum(rho*(this->patch().magSf()*nUp));

        if (estimatedFlowRate/flowRate > 0.5)
        {
            nUp *= (mag(flowRate)/mag(estimatedFlowRate));
        }
        else
        {
            nUp -= ((flowRate - estimatedFlowRate)/gSum(rho*patch().magSf()));
        }

        // Add the corrected normal component of velocity to the patch velocity
        Up += nUp*n;

        // Correct the patch velocity
        this->operator==(Up);

    }
    else
    {
        const scalar avgU = -flowRate_->value(t)/gSum(rho*patch().magSf());
        operator==(avgU*n);
    }
    //////////////////////////////////////////this space is where minegishirei using////////////////

    std::ifstream ifs("input.text");
    std::string inputtext;
    if (ifs.fail())
    {
        Info<<"failed!!!!";
    }
    while (getline(ifs, inputtext))
    {
        Info<<stof(inputtext)<<"\n";
    }
    //////////////////////////////////////////////////////////////////
}


void Foam::myFlowRateFvPatchVectorField::updateCoeffs()
{

    if (updated())
    {
        Info<<"TEST_4_UPDATED\n";
        return;
    }

    if (volumetric_ || rhoName_ == "none")
    {
        updateValues(one());
        Info<<"TEST_5_UPDATEValue\n";
    }
    else
    {
        Info<<"TEST_6_UPDATE_MassFlow\n";
        // Mass flow-rate
        if (db().foundObject<volScalarField>(rhoName_))
        {
            const fvPatchField<scalar>& rhop =
                patch().lookupPatchField<volScalarField, scalar>(rhoName_);

            updateValues(rhop);
        }
        else
        {
            // Use constant density
            if (rhoInlet_ < 0)
            {
                FatalErrorInFunction
                    << "Did not find registered density field " << rhoName_
                    << " and no constant density 'rhoInlet' specified"
                    << exit(FatalError);
            }

            updateValues(rhoInlet_);
        }
    }

    fixedValueFvPatchVectorField::updateCoeffs();
}


void Foam::myFlowRateFvPatchVectorField::write(Ostream& os) const
{
     Info<<"TEST_7_write\n";
    fvPatchField<vector>::write(os);
    flowRate_->writeData(os);
    if (!volumetric_)
    {
        writeEntryIfDifferent<word>(os, "rho", "rho", rhoName_);
        writeEntryIfDifferent<scalar>(os, "rhoInlet", -VGREAT, rhoInlet_);
        
    }
    os.writeKeyword("extrapolateProfile")
        << extrapolateProfile_ << token::END_STATEMENT << nl;
    writeEntry("value", os);
}


// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //

namespace Foam
{
   makePatchTypeField
   (
       fvPatchVectorField,
       myFlowRateFvPatchVectorField
   );
}


// ************************************************************************* //